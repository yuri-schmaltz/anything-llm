# AnythingLLM Server

This package hosts the Express API that powers chat, workspace management, agent automation, and vector database coordination. It exposes REST and WebSocket endpoints consumed by the frontend and orchestrates embeddings with help from the collector service.

## Getting started

```bash
# From the repository root
yarn dev:server
```

Environment variables live in `server/.env.development`. Create it via `yarn setup:envs` and fill out:

- **Core runtime:** `SERVER_PORT`, `DATABASE_URL`, `JWT_SECRET`, `SIG_KEY`, `SIG_SALT`.
- **LLM provider:** `LLM_PROVIDER` plus the provider-specific keys (`OPEN_AI_KEY`, `ANTHROPIC_API_KEY`, etc.).
- **Vector DB:** `VECTOR_DB` and any connector-specific settings (`PINECONE_API_KEY`, `QDRANT_URL`, ...).

## Project structure

- `endpoints/` – REST and WebSocket handlers grouped by feature area.
- `jobs/` – Bree job definitions for recurring background tasks.
- `models/` – Prisma-backed domain helpers used by endpoints and jobs.
- `prisma/` – Schema, migrations, and seeding scripts.
- `storage/` – Default SQLite database location and bundled model assets.
- `utils/` – Provider integrations (LLM, vector DB, speech), logging, security helpers, and shared middlewares.

Swagger documentation can be regenerated with:

```bash
yarn --cwd server swagger
```

## Testing & linting

- Run API-specific tests:
  ```bash
  yarn --cwd server test
  ```
- Format/lint the codebase:
  ```bash
  yarn --cwd server lint
  ```

Both commands are also available via the root shortcuts (`yarn test`, `yarn lint`).

## Extending the server

1. Add new routes under `endpoints/` and wire them into `index.js`.
2. Create or update Prisma models in `prisma/schema.prisma`, then run `yarn prisma:migrate --dev --name <change>` from the root.
3. Use the utilities in `server/utils` to interact with vector databases, providers, or third-party APIs. Keeping integrations in this folder ensures they can be reused by background jobs and the collector service.
4. Update Swagger docs and add Jest coverage for new behaviours.

Refer to [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) for a macro view of how the server interacts with the rest of the system.
