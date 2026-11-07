/* Fable def compiler — C ABI.
 *
 * Compiles the text `.def` / `.tpl` corpus from Fable's debug build into the
 * retail binary format: game.bin, frontend.bin, script.bin, and the shared
 * names.bin.
 *
 * Link against the static library (libdef_compiler_sys.a / def_compiler_sys.lib)
 * or load the shared one (def_compiler_sys.dll / libdef_compiler_sys.so).
 *
 * Thread-safety: defc_build has no shared mutable state and may be called from
 * any thread, including concurrently — though two concurrent builds writing the
 * same output directory will race on the output files.
 */

#ifndef DEF_COMPILER_H
#define DEF_COMPILER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Status codes returned by defc_build. */
#define DEFC_OK           0 /* Build succeeded; log holds warnings + summary.  */
#define DEFC_ERROR_BUILD  1 /* Build failed; log holds the errors and reason.  */
#define DEFC_ERROR_ARGS   2 /* A path was null or not valid UTF-8.            */
#define DEFC_ERROR_PANIC  3 /* Compiler bug; log holds the panic message.     */

/* Compile the def corpus in `input_dir` into the four binaries in `output_dir`.
 *
 *   input_dir   The `Defs/` directory: `.def` / `.tpl` sources plus the `.h`
 *               headers they draw symbols from, scanned recursively. Must be an
 *               existing directory containing at least one def source, or the
 *               call fails rather than emitting empty binaries.
 *   output_dir  Receives game.bin, frontend.bin, script.bin, names.bin.
 *               Created if it does not exist.
 *   log_buf     Receives a NUL-terminated, human-readable log: one
 *               `severity: path:line:col: message` line per diagnostic, then
 *               the outcome. Truncated at a character boundary to fit log_cap.
 *               May be NULL.
 *   log_cap     Size of log_buf in bytes, including the NUL. May be 0.
 *   log_len     Receives the log's full length in bytes, excluding the NUL —
 *               so a caller can size a buffer exactly and call again. May be
 *               NULL.
 *
 * Both paths are interpreted as UTF-8. On Windows that is correct for ASCII
 * paths but not for a non-ASCII path held in an ANSI `std::string`; such a path
 * is rejected with DEFC_ERROR_ARGS rather than silently mis-resolved.
 *
 * Returns one of the DEFC_* status codes. A non-zero return always leaves an
 * explanation in the log.
 */
int32_t defc_build(const char *input_dir,
                   const char *output_dir,
                   char *log_buf,
                   size_t log_cap,
                   size_t *log_len);

/* Compiler version as a static NUL-terminated string. Never NULL; do not free. */
const char *defc_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEF_COMPILER_H */
