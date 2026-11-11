add_subdirectory(auth-helper)
add_subdirectory(system)
# Builds gxde-lock-classic and pulls in lock-common and lock-neo.
add_subdirectory(lock-classic)
# Needs the ext-session-lock protocol target from lock-classic.
add_subdirectory(guard)
