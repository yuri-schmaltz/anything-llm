# AnythingLLM Architecture

AnythingLLM is organised as a JavaScript/TypeScript monorepo that ships three first-class services together with shared assets and deployment tooling. The services communicate over HTTP/WebSocket boundaries and share a SQLite (or configured external) datastore managed through Prisma. This document summarises the architecture to help new contributors understand runtime responsibilities and integration points.

## High-level layout

```
┌─────────────────┐       ┌────────────────────┐       ┌──────────────────────────────┐
│  React Frontend │◄─────►│  Express API Server │◄─────►│ Vector DBs, LLMs, Speech APIs │
└────────┬────────┘       └─────────┬──────────┘       └──────────────┬───────────────┘
         │                            ▲                                │
         │                            │                                │
         ▼                            │                                │
┌─────────────────┐       ┌───────────┴──────────┐       ┌──────────────┴──────────────┐
│  Collector API  │──────►│ Prisma ORM + SQLite │       │ Background jobs / schedulers │
└─────────────────┘       └─────────────────────┘       └──────────────────────────────┘
```

- **frontend/** – Vite + React application that renders the management UI, chat experience, and administration surfaces.
- **server/** – Express server that exposes the user/admin API, orchestrates conversations, triggers agent flows, and coordinates vector database operations.
- **collector/** – Express worker service responsible for ingesting, chunking, and embedding user supplied documents before they become searchable context.
- **docker/**, **cloud-deployments/**, and **extras/** – Infrastructure templates, container assets, and desktop distribution tooling that deploy the trio together.

## Runtime responsibilities

### Frontend
- Provides authentication flows and workspace management screens.
- Streams chat responses over WebSocket connections exposed by the API server.
- Ships static assets built via Vite (`yarn dev` for development, `yarn build` for production).

### API Server
- Hosts REST/WS endpoints under `server/endpoints` and Swagger docs inside `server/swagger`.
- Persists metadata and workspace configuration through Prisma models defined in `server/prisma/schema.prisma`.
- Brokers requests to LLM providers, embedders, vector databases, and speech services. Provider integrations live under `server/utils`.
- Executes background tasks using Bree jobs defined in `server/jobs` (e.g., scheduled syncs, cleanup tasks).

### Collector
- Accepts document uploads and link ingestion requests exposed from the frontend UI.
- Normalises raw content into token-sized chunks compatible with selected embedding models.
- Stores temporary artefacts under `collector/storage` before handing them to the API server.
- Shares Prisma client definitions with the API server to read/write workspace data.

## Data & state flow

1. **Ingestion** – The frontend uploads files or URLs to the collector. The collector queues processing jobs and emits structured chunks.
2. **Embedding** – The API server receives the processed chunks, generates embeddings using the configured provider, and persists them in the vector database of choice.
3. **Conversation** – When a user chats, the frontend calls the API server which retrieves relevant context from the vector database and asks the configured LLM for a response.
4. **Agents & automations** – Agent flows and background jobs (e.g., MCP connectors, scheduled syncs) run inside the server but leverage shared utilities to talk to external systems.
5. **Storage** – By default Prisma targets the bundled SQLite database at `server/storage/anythingllm.db`. Production deployments can point Prisma to Postgres, MySQL, MSSQL, etc., through environment variables.

## External integrations

- **LLMs & Embedders** – Providers are configured via `server/.env.development` (OpenAI, Anthropic, Groq, Ollama, etc.). Each integration resides under `server/utils/AiProviders`.
- **Vector databases** – Pluggable connectors for LanceDB, PGVector, Pinecone, Weaviate, Qdrant, Milvus, and more live under `server/utils/vectorDbProviders` with dedicated setup guides.
- **Speech services** – Text-to-speech and speech-to-text support is wired into the frontend and server, toggled by environment variables.

## Observability and tooling

- Logging uses Winston with environment-aware configuration (see `server/utils/logger`).
- Swagger documentation can be regenerated via `yarn --cwd server swagger`.
- Background tasks use Bree (`server/jobs`) and share queue state with the main Express instance.

## Deployments

- **Local development** – Use `yarn setup` followed by `yarn dev:all` (or per-service commands) to run everything locally.
- **Docker** – `docker/` contains Dockerfiles, compose definitions, and `.env` templates for containerized deployments.
- **Cloud templates** – `cloud-deployments/` provides AWS CloudFormation and GCP Deployment Manager scripts generated via root npm commands.
- **Desktop** – `extras/qt-installer/` packages the application with a Qt-based installer for Windows/macOS/Linux.

For deeper dives into developer workflows and configuration see [`docs/DEVELOPMENT_WORKFLOW.md`](./DEVELOPMENT_WORKFLOW.md) and [`docs/CONFIGURATION.md`](./CONFIGURATION.md).
