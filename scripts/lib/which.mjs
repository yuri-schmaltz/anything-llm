import { join } from "node:path";
import fs from "node:fs";

export function which(binary) {
  const delimiter = process.platform === "win32" ? ";" : ":";
  const extensions = process.platform === "win32" ? [".cmd", ".exe", ".bat"] : [""];
  const paths = process.env.PATH?.split(delimiter) ?? [];
  for (const base of paths) {
    for (const extension of extensions) {
      const full = join(base, `${binary}${extension}`);
      if (fs.existsSync(full)) return full;
    }
  }
  return null;
}
