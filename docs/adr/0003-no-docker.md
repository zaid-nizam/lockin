# 0003: No Docker image

## Context
The project template ships Docker and Compose for every app.

## Decision
None here.

## Consequences
The build is `make build` and the artefact is one 92 KB binary. The tests
get their isolation from `unshare -m` instead of a container.

## Alternatives rejected
A container cannot usefully bind-mount over the host's `/etc/hosts` or kill
the host user's processes without being given the host; at that point it is
the host with extra steps.
