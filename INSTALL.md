## CMake installation instructions

gxde-lock requires CMake 3.21 or newer, a C++20 compiler, Qt 6.5+ and
DTK2Widget-Qt6. The complete package dependency list is maintained in
`debian/control`.

Configure and build from the project root:

```sh
cmake -S . -B build-cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-cmake -j4
```

The default build includes both lock screens (`gxde-lock-classic`,
`gxde-lock-neo`), the authentication helper and the root helper. Stage the
install tree before installing it onto the host:

```sh
DESTDIR="$PWD/build-cmake/stage" cmake --install build-cmake
```

After inspecting `build-cmake/stage`, install it with:

```sh
sudo cmake --install build-cmake
```

Useful project options include:

- `BUILD_NEO_LOCK=OFF` builds only the classic lock screen.
- `LOCK_BUILD_TESTS=ON` builds the lock backend tests.
- `ENABLE_JOURNALD=OFF` disables journald logging.
- `INSTALL_PAM_CONFIGURATION=OFF` skips installation of PAM configuration.

Run `cmake -S . -B build-cmake -LAH` to list all cached options. CMake installs
files directly and does not produce a Debian package; use `./build-deb -d` for
the package-managed installation path.
