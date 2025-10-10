import { relative } from "node:path";
import { rootDir } from "../config.mjs";

export function printHeader(message) {
  const line = "-".repeat(message.length);
  console.log(`\n${message}\n${line}`);
}

export function relPath(path) {
  return relative(rootDir, path);
}

export function logCheckResult(name, passed, detail) {
  const symbol = passed ? "✔" : "✖";
  console.log(`${symbol} ${name}`);
  if (!passed && detail) {
    console.log(`    ${detail}`);
  }
}
