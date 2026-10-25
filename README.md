# EterLib (`pkg_eterlib`)

Core engine framework library for the Open-MT2 client (`x86_64` Linux & Windows).

## Overview

`EterLib` provides the foundational engine subsystems used across the Metin2 client:

- **Graphics & Rendering Abstraction**: `CGraphicBase`, `CGraphicDevice`, `CScreen`, `CStateManager`, `CGraphicTexture`, `CGraphicImageTexture`, `CGraphicFontTexture`, `CGraphicVertexBuffer`, `CGraphicIndexBuffer`, `CGraphicVertexShader`, `CGraphicPixelShader`, `CGraphicDib`.
- **Scene & Spatial Management**: `CCamera`, `CCameraManager`, `CCullingManager`, `CGraphicCollisionObject`, `CSphereCollisionInstance`, `CAABBCollisionInstance`, `COBBCollisionInstance`, `CRay`, `CSkyBox`, `CLensFlare`, `CDecal`.
- **Networking**: `CNetworkStream`, `CNetworkAddress`, `CNetworkDatagram`, `CNetDatagramSender`, `CNetDatagramReceiver`, `CNetworkPacketHeaderMap`.
- **Threading & Resource Loading**: `CThread`, `Mutex`, `CFileLoaderThread`, `CResource`, `CResourceManager`, `CTextFileLoader`, `CMemoryTextFileLoader`.
- **Windowing, Input & Text/IME**: `CMSWindow`, `CMSApplication`, `CInputDevice`, `CInputKeyboard`, `CInputMouse`, `CIME`, `TextTag`.
- **Memory & Containers**: `CDynamicPool`, `CDynamicPoolEx`, `CPooledObject`, `CReferenceObject`, `CRef`.

## Directory Structure (Pitchfork Layout)

```text
EterLib/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── EterLib/          # Public EterLib headers
│   └── Platform/         # Cross-platform POSIX/Win32 & DirectXMath compatibility headers
├── src/                  # Implementation files (.cpp)
└── tests/                # CTest smoke and unit tests
```

## Building & Testing

### Standalone Build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Monorepo Build

Included automatically via `add_subdirectory(src/EterLib)` from the root `CMakeLists.txt`.
