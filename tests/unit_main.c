#include "suites.h"
#include "test.h"

int main(void) {
    suite_text();
    suite_records();
    suite_config();
    suite_stats();
    suite_canvas();
    suite_widgets();
    suite_store();
    suite_controls();
    suite_app();
    suite_app_end();
    suite_screens();
    return test_summary();
}
