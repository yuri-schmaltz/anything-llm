export function parseArgs(argv) {
  const [command, ...rest] = argv;
  const flags = new Map();
  const positional = [];

  for (let i = 0; i < rest.length; i += 1) {
    const current = rest[i];
    if (!current.startsWith("--")) {
      positional.push(current);
      continue;
    }

    const flagBody = current.slice(2);
    const [key, inlineValue] = flagBody.split("=");
    if (inlineValue !== undefined) {
      flags.set(key, inlineValue);
      continue;
    }

    const next = rest[i + 1];
    if (next && !next.startsWith("--")) {
      flags.set(key, next);
      i += 1;
    } else {
      flags.set(key, true);
    }
  }

  return { command, flags, positional };
}
