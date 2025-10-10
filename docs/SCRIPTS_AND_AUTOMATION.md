# Scripts & Automation

AnythingLLM exposes a unified task runner at `scripts/manage.mjs`. All root `yarn` commands are thin wrappers around this script
, which keeps service workflows consistent across the monorepo. This document describes the available commands and recommended f
lags.

## Quick reference

| Task | Command | Notes |
| --- | --- | --- |
| Install dependencies | `yarn install:packages` | Uses `scripts/manage.mjs install`. Add `--force` to reinstall. |
| Copy `.env` templates | `yarn setup:envs` | Idempotently copies example files without overwriting manual edits. |
| Full setup | `yarn setup` | Installs deps, copies envs, runs Prisma generate/migrate/seed. Use `--skip-prisma` or `--force-install`. |
| Run services together | `yarn dev:all` | Spawns frontend, server, and collector with linked logging. |
| Prisma utilities | `yarn prisma:<task>` | Wraps `npx prisma` with the correct working directory. |
| Translation hygiene | `yarn normalize:translations` | Normalises, verifies, and (by default) lints locale files. Add `--no-lint` to skip linting. |
| Environment checks | `yarn doctor` | Verifies Node version, tooling availability, directories, and `.env` files. |
| Cleanup | `yarn clean` | Removes `node_modules` from each package to allow a clean reinstall. |

## Flags

Many commands accept extra flags:

- `yarn setup --skip-prisma` – Prepare dependencies and `.env` files without touching the database.
- `yarn setup --force-install` – Force reinstall dependencies even if `node_modules` is present.
- `yarn install:packages --force` – Force reinstalling dependencies without touching env/prisma tasks.
- `yarn prisma:migrate --dev --name my_migration` – Run `prisma migrate dev` with a custom migration name.
- `yarn normalize:translations --no-lint` – Speed up locale tweaks when you do not need to run the full lint suite.
- `yarn doctor --verbose` – Print additional hints about missing `.env` files.

## Adding new tasks

1. Implement the functionality inside `scripts/manage.mjs` (or extract helpers into `scripts/lib/`).
2. Register a `yarn` alias in the root `package.json` if the command should be user-facing.
3. Update this document when new workflows are introduced so the script catalogue stays discoverable.

See [`docs/DEVELOPMENT_WORKFLOW.md`](./DEVELOPMENT_WORKFLOW.md) for a broader overview of developer tooling and debugging strate
gies.
