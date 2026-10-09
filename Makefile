# Every command a developer needs. `make` alone lists them.
.DEFAULT_GOAL := help
.PHONY: help doctor setup build dev test test-e2e coverage lint fmt fmt-check size scan docs \
        check install uninstall clean

CC      ?= cc
PREFIX  ?= /usr/local
CFLAGS  := -std=c17 -D_GNU_SOURCE -Isrc -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion \
           -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wvla -Wcast-qual -Wundef
# -rdynamic keeps function names in the binary so a crash trace is readable.
LDFLAGS := -rdynamic
LDLIBS  := -lX11

PORTABLE := $(wildcard src/core/*.c src/ui/*.c) $(filter-out src/app/main.c,$(wildcard src/app/*.c))
LINUX    := $(wildcard src/sys/*.c) src/platform/platform_x11.c
TESTS    := $(filter-out tests/linux_tests.c tests/x11_send.c,$(wildcard tests/*.c))
FORMAT   := $(filter-out src/ui/font_data.c,$(wildcard src/*/*.c src/*/*.h tests/*.c tests/*.h))

# The tests that touch /etc/hosts, switch users and open windows need root.
# They run inside `unshare -m`, so the real hosts file is never changed.
SUDO := $(if $(filter 0,$(shell id -u)),,sudo)

comma := ,

# One object tree per build flavour: release, asan (tests) and cov (coverage).
define flavour
build/$(1)/%.o: %.c
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) -Itests $(2) -MMD -MP -c $$< -o $$@
build/$(1)/lockin: $$(patsubst %.c,build/$(1)/%.o,$$(PORTABLE) $$(LINUX) src/app/main.c)
	$$(CC) $(2) $$(LDFLAGS) $$^ $$(LDLIBS) -o $$@
build/$(1)/unit: $$(patsubst %.c,build/$(1)/%.o,$$(PORTABLE) $$(TESTS))
	$$(CC) $(2) $$^ -o $$@
build/$(1)/linux_tests: $$(patsubst %.c,build/$(1)/%.o,$$(wildcard src/core/*.c) $$(wildcard src/sys/*.c) tests/linux_tests.c tests/test.c)
	$$(CC) $(2) $$(LDFLAGS) $$^ -o $$@
build/$(1)/x11_send: tests/x11_send.c
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$< $$(LDLIBS) -o $$@
endef
$(eval $(call flavour,release,-O2 -g -fno-omit-frame-pointer))
$(eval $(call flavour,asan,-O1 -g -fno-omit-frame-pointer -fsanitize=address$(comma)undefined -fno-sanitize-recover=all))
$(eval $(call flavour,cov,-O0 -g --coverage -DLOCKIN_COVERAGE))
-include $(shell find build -name '*.d' 2>/dev/null)

help: ## List every command
	@grep -E '^[a-z0-9-]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  %-10s %s\n", $$1, $$2}'

doctor: ## Check required tools and versions, and say how to fix what is missing
	@./scripts/doctor.sh

setup: doctor ## First run: install the git hook that formats and lints
	@git config core.hooksPath .githooks 2>/dev/null || true
	@echo "Ready. Build with: make build"

build: build/release/lockin ## Build the program at build/release/lockin

dev: build ## Run the program without root: the window opens, blocking is off
	./build/release/lockin

test: build/asan/unit ## Unit tests under AddressSanitizer and UBSan, three runs
	@for run in 1 2 3; do ./build/asan/unit 2>build/asan/unit.stderr \
	  || { cat build/asan/unit.stderr; exit 1; }; done

test-e2e: build/asan/lockin build/asan/linux_tests build/asan/x11_send ## System and window tests (asks for sudo)
	$(SUDO) unshare -m ./tests/system.sh build/asan
	$(SUDO) unshare -m ./tests/e2e.sh build/asan

coverage: build/cov/lockin build/cov/unit build/cov/linux_tests build/cov/x11_send ## All tests on an instrumented build; fails under 95% (asks for sudo)
	@find build/cov -name '*.gcda' -delete
	./build/cov/unit 2>/dev/null
	$(SUDO) unshare -m ./tests/system.sh build/cov
	$(SUDO) unshare -m ./tests/e2e.sh build/cov
	gcovr --root . --object-directory build/cov --filter 'src/' --exclude 'src/ui/font_data.c' \
	  --exclude-unreachable-branches --fail-under-line 95 --fail-under-branch 95 \
	  --txt build/cov/coverage.txt --print-summary
	@tail -n +1 build/cov/coverage.txt | grep -v '100%' | head -40

fmt: ## Format all code
	clang-format -i $(FORMAT)

fmt-check:
	clang-format --dry-run -Werror $(FORMAT)

lint: ## clang-tidy, cppcheck and shellcheck with warnings as errors
	@# stderr carries only a per-file count of suppressed system-header notes.
	clang-tidy --quiet $(filter %.c,$(FORMAT)) -- $(CFLAGS) -Itests 2>/dev/null
	cppcheck --quiet --std=c11 --enable=warning,style,performance,portability --inline-suppr \
	  --suppress=missingIncludeSystem --error-exitcode=1 -Isrc -Itests src tests
	shellcheck scripts/*.sh tests/*.sh .githooks/pre-commit

size: ## Size limits: function 40 lines, 4 parameters, complexity 10, file 300 lines
	./scripts/size.sh

scan: ## Secret scan and static security analysis
	./scripts/scan.sh

docs: ## Regenerate src/ui/font_data.c from the X11 bitmap fonts
	./tools/genfont.py /usr/share/fonts/X11/75dpi/helvR12-ISO8859-1.pcf.gz \
	  /usr/share/fonts/X11/75dpi/helvB12-ISO8859-1.pcf.gz > src/ui/font_data.c

check: fmt-check lint size build test test-e2e coverage scan ## Everything CI runs

install: build ## Install to $(PREFIX)/bin (run with sudo)
	install -D -m 0755 build/release/lockin $(DESTDIR)$(PREFIX)/bin/lockin

uninstall: ## Remove the installed program
	rm -f $(DESTDIR)$(PREFIX)/bin/lockin

clean: ## Delete build output
	rm -rf build
