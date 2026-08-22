#pragma once

#include <QString>

/**
 * Deterministic, fail-closed policy for commands Workspace Auto may run without
 * an approval card. The command is executed only from the already-contained
 * project root; every command outside this narrow local build/test/read-only
 * set must be reviewed explicitly.
 */
namespace AiWorkspacePolicy {

[[nodiscard]] bool commandRequiresApproval(const QString& command, QString* reason = nullptr);

} // namespace AiWorkspacePolicy
