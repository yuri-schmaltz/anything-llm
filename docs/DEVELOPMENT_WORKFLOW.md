# Development Workflow

The AnythingLLM codebase is a monorepo with three active Node.js workspaces (`server`, `collector`, `frontend`). This guide covers the recommended day-to-day workflow for contributors.

## 1. Prerequisites

- Node.js ≥ 18 (the doctor command verifies this).
- Yarn (Classic) – included in the repository via the `engines` field.
- SQLite (bundled) or access to your preferred database engine if overriding `DATABASE_URL`.

Run a quick diagnostic with:

```bash
yarn doctor
```

## 2. Initial setup

```bash
yarn setup
```

This command performs three actions:

1. Installs dependencies for `server`, `collector`, and `frontend`.
2. Copies `.env` templates into each workspace (existing files are preserved).
3. Generates the Prisma client, applies migrations, and seeds the database.

Need to skip Prisma when working offline? Use:

```bash
yarn setup --skip-prisma
```

Only need dependencies? Run:

```bash
yarn install:packages
```

Add `--force` to reinstall even when `node_modules` already exists.

## 3. Running the stack locally

Choose the approach that fits your preference:

- All services with shared logging:
  ```bash
  yarn dev:all
  ```
- Individual services in separate terminals:
  ```bash
  yarn dev:server
  yarn dev:collector
  yarn dev:frontend
  ```

The frontend expects the API server at `http://localhost:3001` and the collector at `http://localhost:8889` by default. Adjust values in `frontend/.env` if you change ports.

## 4. Database workflows

- Create a new migration (development only):
  ```bash
  yarn prisma:migrate --dev --name add-new-table
  ```
- Recreate the SQLite database from scratch:
  ```bash
  yarn prisma:reset
  ```
- Seed reference data manually:
  ```bash
  yarn prisma:seed
  ```

All Prisma commands run inside the `server` workspace but are available from the repository root.

## 5. Linting & formatting

Run the unified lint task before opening a pull request:

```bash
yarn lint
```

Each workspace maintains its own ESLint/Prettier configuration. The root command executes them sequentially and fails fast on errors.

## 6. Testing

The repository contains Jest suites in the `server` and `collector` packages. Execute them with:

```bash
yarn test
```

Add package-specific tests by placing files in the respective `__tests__` directories. Continuous integration runs this script automatically.

## 7. Updating translations

Normalise English strings and verify locale completeness with:

```bash
yarn normalize:translations
```

This runs the localisation utilities, triggers the verification script, and lints the workspaces. Pass `--no-lint` to skip the lint step during rapid iteration.

## 8. Useful tips

- Use `yarn doctor` whenever switching machines to confirm your environment is ready.
- `yarn clean` removes `node_modules` folders if you need a clean slate before reinstalling dependencies.
- `server/nodemon.json` and `collector/nodemon.json` watch the appropriate directories—feel free to extend them when adding new folders.
- Swagger docs can be regenerated via `yarn --cwd server swagger` to reflect endpoint changes.
- Deployment templates are generated with `yarn generate:cloudformation` and `yarn generate:gcp-deployment`.

Refer back to [`docs/ARCHITECTURE.md`](./ARCHITECTURE.md) for an overview of how the pieces fit together, [`docs/CONFIGURATION.md`](./CONFIGURATION.md) when modifying environment variables, and [`docs/SCRIPTS_AND_AUTOMATION.md`](./SCRIPTS_AND_AUTOMATION.md) for a quick reference of available commands.
