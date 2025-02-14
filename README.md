# Graphic Engine

이 프로젝트는 **DirectX 12** 기반의 간단한 그래픽 엔진 예제입니다.  
라이브러리 의존성 관리를 위해 [vcpkg](https://github.com/microsoft/vcpkg)를 사용하며,  
`vcpkg.json`(Manifest 모드)를 통해 필요한 라이브러리를 자동으로 설치하고 빌드할 수 있습니다.

---

## 개요

- **DirectXTex**: 이미지(텍스처) 로딩/처리, OpenEXR 지원
- **DirectXTK12**: DirectX 12 도우미 툴킷
- **Assimp**: 3D 모델 포맷 임포팅

해당 라이브러리들은 모두 `vcpkg.json` 내 의존성 목록을 통해 자동 관리됩니다.

-- **ImGui** : GUI 라이브러리(프로젝트 내 포함됨)

---

## 필수 요구사항

- **Windows 10** 이상 (DirectX 12 지원 환경)
- **CMake 3.21** 이상 (권장)  
  또는 **Visual Studio 2022**
- **C++17** 이상
- **vcpkg** 

---

## Assets

- https://www.cgtrader.com/items/4693538/download-page
- https://freepbr.com/product/vented-metal-panel1/
- https://freepbr.com/product/worn-painted-metal/