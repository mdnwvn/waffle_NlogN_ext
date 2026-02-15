# Building the Waffle proxy binary

## Environment

This build process has been tested on:

- Ubuntu 22.04 (x86_64)
- GCC/ G++ 11.4.0
- CMake 3.22.1

## Dependencies

- Manually installed: `build-essential cmake libssl-dev pkg-config`

## Diff(s) from `sharathvemula/waffle_test`:

**cpp_redis** compatibility patch:

Modify the following file:
```
waffle_NlogN_ext/waffle/cmake-modules/Dependencies.cmake
```

Locate the line:
```
set(CPP_REDIS_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
```

Replace it with:
```
set(CPP_REDIS_CXX_FLAGS "${EXTERNAL_CXX_FLAGS} -include thread")
```

## Build

In `waffle_NlogN_ext/waffle`:
```sh
sh build.sh
```

## Potential Issues/ Troubleshooting

- \[Debian 13\]: libevent / arc4random_addrandom (3) error
