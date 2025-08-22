FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential wget ca-certificates curl git python3 python3-pip cmake \
    libbz2-dev libzstd-dev liblzma-dev libssl-dev pkg-config ccache lsb-release \
    gnupg software-properties-common && \
    rm -rf /var/lib/apt/lists/*

# 安装 clang-20
RUN wget https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh && chmod +x /tmp/llvm.sh && \
    /tmp/llvm.sh 20 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-20 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-20 100 && \
    update-alternatives --set cc /usr/bin/clang-20 && update-alternatives --set c++ /usr/bin/clang++-20

# 可选：安装 CMake 3.28（若系统自带版本足够可删）
RUN wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.sh -O /tmp/cmake.sh && \
    chmod +x /tmp/cmake.sh && /tmp/cmake.sh --skip-license --prefix=/usr/local

# 编译并安装 Boost 1.89
RUN wget https://sourceforge.net/projects/boost/files/boost/1.89.0/boost_1_89_0.tar.gz -O /tmp/boost_1_89_0.tar.gz && \
    mkdir -p /tmp/boost && tar -xzf /tmp/boost_1_89_0.tar.gz -C /tmp/boost --strip-components=1 && \
    cd /tmp/boost && ./bootstrap.sh && ./b2 install --prefix=/usr/local -j$(nproc) && \
    rm -rf /tmp/boost /tmp/boost_1_89_0.tar.gz

RUN wget -O /tmp/googletest-1.17.0.tar.gz https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz && \
    mkdir -p /tmp/gtest && tar -xzf /tmp/googletest-1.17.0.tar.gz -C /tmp/gtest --strip-components=1 && \
    mkdir -p /tmp/gtest/build && cd /tmp/gtest/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --target install -j$(nproc) && \
    ldconfig && \
    rm -rf /tmp/gtest /tmp/googletest-1.17.0.tar.gz

# 清理
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/*