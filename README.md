# Gruut-Enterprise Merger Project
[![Build Status](https://travis-ci.com/gruut/enterprise-merger.svg?branch=develop)](https://travis-ci.com/gruut/enterprise-merger)

### Prerequisite
  - CMake (**3.12**)
  - vcpkg (**optional**)
    * 라이브러리 패키지 매니저(https://github.com/Microsoft/vcpkg)
  - clang-tidy
    * CLion -> Preference -> Inspection -> Clang-Tidy -> Option에서 `Use IDE setting`을 해제
  - **grpc**
    * 각자 라이브러리를 설치해야 합니다.
    * [링크](https://github.com/grpc/grpc)
  - **LevelDB**
    * 각자 라이브러리를 설치해야 합니다.
    * [링크](https://github.com/google/leveldb)
  - **lz4**
    * 각자 라이브러리를 설치해야 합니다.
  - **botan**
    * 각자 라이브러리를 설치해야 합니다.
    * [링크](https://github.com/randombit/botan)


### [Project Directory Structure](https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/)
  - build: This folder contains all object files, and is removed on a clean.
  - include: All project header files. All necessary third-party header files that do not exist under /usr/local/include are also placed here.
  - src: The application and only the application’s source files.
  - tests: All test code files.
  - scripts: script files. i.e., run-clang-tidy 
  
### Code Convention
  - .clang-tidy 파일에 명세한대로 변수명, 클래스명 등을 작성해주세요. 
    * local에서 테스트하는 방법
       * `python3 scripts/run-clang-tidy`
    * 기본적으로 CLion에서 체크해주고, Gitlab에서도 소스코드를 push할때 검사하도록 설정했습니다.
  - Git Branches
    * `master` 브랜치: 최종적으로 production에 반영될 브랜치입니다. 절대 `master`에서 작업하면 안됩니다.
    * `develop` 브랜치: 기본적으로 사용될 브랜치입니다. 항상 작업하기전에 `git pull` 명령어를 통해 브랜치를 최신화하고, 자신의 작업 브랜치를 새로 생성합니다.
    * `feature/*` 브랜치: `develop`에서 `git checkout -B feature/*(zsh에서는 gcb로 간단하게 실행할 수 있음)` 후에 새로운 브랜치에서 작업을 합니다.
  - Work Flow
    * 위에서도 언급했듯 개발자는 `develop`에서 `git pull`를 통해 소스코드를 최신화하고, `feature/*` 브랜치를 생성해서 작업을 시작합니다.
    * 작업이 완료되면 `featrue/*` -> `develop`으로 Merge 할 것이라는 Merge Request를 Gitlab에 올려야 합니다.    
  - 개발자간 반드시 지켜야할 것
    * 읽기 좋은 코드가 좋은 코드입니다.
      * 자신만 알수있는 코드를 작성하지 말아주세요. (함수명, 변수명 등등) 
      * 시간 나실때 읽어보세요. [책 링크](http://www.yes24.com/24/goods/6692314?scode=032&OzSrank=1)  
    * 주석은 되도록 코드에 작성하지 말아주세요.
      * commit 메시지에 필요한 내용들을 작성해두면 나중에 참조할 수 있기 때문에 주석은 되도록 작성하지 말아주세요.
      * 주석은 코드를 지우면 사라지지만, commit 메시지는 `git blame`을 통해 시간이 지나도 확인할 수 있기 때문입니다. 
      * 복잡한 알고리즘을 설명할때는 예외로 합니다.
    * Merge Request는 다른 개발자들이 이해할 수 있게 작성해주세요.    
