#!/usr/bin/env bash

# This script builds gxde-lock & its deployable install tarball with bazel.
#
# Qt6 devel packages are required and we ASSUME that you already satisified
# those requirement.
# See MODULE.bazel and bazel/qt/system_qt.bzl for more details.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

bazel build "$@" //:gxde-lock-dist

echo
echo "Build shall be complete. Here're the artifacts:"
echo "\tlockers: bazel-bin/src/lock-classic/gxde-lock-classic, bazel-bin/src/lock-neo/gxde-lock-neo"
echo "\thelpers: bazel-bin/src/auth-helper/gxde-lock-auth-helper, bazel-bin/src/system/gxde-lock-system"
echo "\ttarball: bazel-bin/gxde-lock-dist.tar  (extract at / on the target)"
