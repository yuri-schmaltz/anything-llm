import { access, constants, copyFile, mkdir, rm } from "node:fs/promises";
import { dirname } from "node:path";

export async function fileExists(path) {
  try {
    await access(path, constants.F_OK);
    return true;
  } catch {
    return false;
  }
}

export async function ensureDirectory(path) {
  await mkdir(path, { recursive: true });
}

export async function ensureFileFromTemplate(examplePath, targetPath) {
  await ensureDirectory(dirname(targetPath));
  await copyFile(examplePath, targetPath, constants.COPYFILE_EXCL).catch((error) => {
    if (error.code === "EEXIST") return;
    throw error;
  });
}

export async function removeIfExists(path) {
  await rm(path, { force: true, recursive: false });
}
