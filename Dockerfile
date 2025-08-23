FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential wget ca-certificates curl git python3 python3-pip \
    libbz2-dev libzstd-dev liblzma-dev libssl-dev pkg-config ccache lsb-release \
    gnupg software-properties-common unzip gcc g++ && \
    rm -rf /var/lib/apt/lists/*

# 安装 clang-20
RUN wget https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh && chmod +x /tmp/llvm.sh && \
    /tmp/llvm.sh 20 && \
    apt-get update && apt-get install -y \
    libc++-20-dev libc++abi-20-dev && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang-20 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-20 100 && \
    update-alternatives --set cc /usr/bin/clang-20 && update-alternatives --set c++ /usr/bin/clang++-20

# 设置编译器环境变量为 LLVM 20
ENV CC=/usr/bin/clang-20
ENV CXX=/usr/bin/clang++-20

# 构建并安装最新版本的 CMake
RUN wget https://github.com/Kitware/CMake/archive/refs/tags/v4.1.0.tar.gz -O /tmp/cmake.tar.gz && \
    mkdir -p /tmp/cmake && tar -xzf /tmp/cmake.tar.gz -C /tmp/cmake --strip-components=1 && \
    cd /tmp/cmake && ./bootstrap && make -j$(nproc) && make install && \
    rm -rf /tmp/cmake /tmp/cmake.tar.gz

# 编译并安装 Boost 1.89
RUN wget https://sourceforge.net/projects/boost/files/boost/1.89.0/boost_1_89_0.tar.gz -O /tmp/boost_1_89_0.tar.gz && \
    mkdir -p /tmp/boost && tar -xzf /tmp/boost_1_89_0.tar.gz -C /tmp/boost --strip-components=1 && \
    cd /tmp/boost && ./bootstrap.sh && ./b2 install --prefix=/usr/local -j$(nproc) && \
    rm -rf /tmp/boost /tmp/boost_1_89_0.tar.gz

# 编译并安装 GoogleTest 1.17.0
RUN wget -O /tmp/googletest-1.17.0.tar.gz https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz && \
    mkdir -p /tmp/gtest && tar -xzf /tmp/googletest-1.17.0.tar.gz -C /tmp/gtest --strip-components=1 && \
    mkdir -p /tmp/gtest/build && cd /tmp/gtest/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --target install -j$(nproc) && \
    ldconfig && \
    rm -rf /tmp/gtest /tmp/googletest-1.17.0.tar.gz

# 安装 Ninja 1.11.1
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip -O /tmp/ninja-linux.zip && \
    unzip /tmp/ninja-linux.zip -d /usr/local/bin && \
    chmod +x /usr/local/bin/ninja && \
    rm /tmp/ninja-linux.zip

# 清理
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/*