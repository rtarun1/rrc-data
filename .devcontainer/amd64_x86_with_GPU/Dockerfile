FROM nvidia/cuda:12.2.2-devel-ubuntu22.04

# basic args
ARG ROS_DISTRO=humble
ARG USERNAME=container_user
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Add the following labels
LABEL org.opencontainers.image.description="Data collection"
LABEL org.opencontainers.image.title="RRCData"
LABEL org.opencontainers.image.vendor="Tarun R"
LABEL org.opencontainers.image.source="https://github.com/rtarun1/rrc-data"
LABEL maintainer="tarun.ramak@gmail.com"
LABEL org.opencontainers.image.licenses="MIT"

# handle default shell
SHELL ["/bin/bash", "-c"]
ENV SHELL=/bin/bash

# setup timezone
RUN echo 'Asia/Kolkata' > /etc/timezone && \
    ln -s /usr/share/zoneinfo/Asia/Kolkata /etc/localtime && \
    apt-get update && \
    apt-get install -q -y --no-install-recommends tzdata && \
    rm -rf /var/lib/apt/lists/*

# install packages
RUN apt-get update && apt-get install -q -y --no-install-recommends \
    dirmngr \
    gnupg2 \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Disable apt-get warnings
RUN apt-get update || true && apt-get install -y --no-install-recommends apt-utils dialog curl && \
  rm -rf /var/lib/apt/lists/*

# Setup ROS Apt sources
RUN curl -L -s -o /tmp/ros2-apt-source.deb https://github.com/ros-infrastructure/ros-apt-source/releases/download/1.1.0/ros2-apt-source_1.1.0.jammy_all.deb \
    && echo "1600cb8cc28258a39bffc1736a75bcbf52d1f2db371a4d020c1b187d2a5a083b /tmp/ros2-apt-source.deb" | sha256sum --strict --check \
    && apt-get update \
    && apt-get install /tmp/ros2-apt-source.deb \
    && rm -f /tmp/ros2-apt-source.deb \
    && rm -rf /var/lib/apt/lists/*

# setup environment
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Create non root user with sudo privilege
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME
USER $USERNAME

ENV ROS_DISTRO=$ROS_DISTRO

# install ros2 packages
RUN sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    ros-$ROS_DISTRO-desktop \
    && sudo rm -rf /var/lib/apt/lists/*

# setup entrypoint
COPY ./ros_entrypoint.sh /

ENTRYPOINT ["/ros_entrypoint.sh"]

# install bootstrap tools
RUN sudo apt-get update && sudo apt-get install --no-install-recommends -y \
    build-essential \
    git \
    bash-completion \
    python3-colcon-common-extensions \
    python3-colcon-mixin \
    python3-rosdep \
    python3-vcstool \
    python3-pip \
    && sudo rm -rf /var/lib/apt/lists/*

# bootstrap rosdep
RUN sudo rosdep init && \
    rosdep update --rosdistro $ROS_DISTRO

# setup colcon mixin and metadata
RUN sudo colcon mixin add default \
    https://raw.githubusercontent.com/colcon/colcon-mixin-repository/master/index.yaml && \
    sudo colcon mixin update && \
    sudo colcon metadata add default \
    https://raw.githubusercontent.com/colcon/colcon-metadata-repository/master/index.yaml && \
    sudo colcon metadata update

RUN sudo pip3 install -U \
    argcomplete

# echo sources
RUN echo 'source /usr/share/bash-completion/bash_completion' | sudo tee -a ~/.bashrc > /dev/null && \
    echo 'source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash' | sudo tee -a ~/.bashrc > /dev/null && \
    echo "source /opt/ros/${ROS_DISTRO}/setup.bash" | sudo tee -a ~/.bashrc > /dev/null && \
    echo 'eval "$(register-python-argcomplete3 ros2)"' | sudo tee -a ~/.bashrc > /dev/null && \
    echo 'eval "$(register-python-argcomplete3 colcon)"' | sudo tee -a ~/.bashrc > /dev/null

# Install deps
RUN sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    gnupg \
    software-properties-common \
    libtiff5 \
    libegl1 \
    libxext6 \
    libvulkan1 \
    freeglut3-dev \
    vulkan-tools \
    libdc1394-25 \
    mesa-utils \
    libssl-dev \
    libusb-1.0-0-dev \
    libudev-dev \
    pkg-config \
    libgtk-3-dev \
    cmake \
    build-essential \
    ssh \
    udev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libaprutil1-dev \
    at \
    v4l-utils \
    xterm \
    tmux \
    vim \
    nano \
    tree \
    python3.10-venv \
    wget \
    ffmpeg=7:* \
    libsm6=2:* \
    libxext6=2:* \
    git=1:* \
    ninja-build \
    gcc-10 \
    g++-10 \
    && sudo apt-get autoremove -y \
    && sudo apt-get clean -y \
    && sudo rm -rf /var/lib/apt/lists/*

# Setup NVIDIA Container Toolkit for GPU access
# RUN VULKAN_API_VERSION=$(dpkg -s libvulkan1 | grep -oP 'Version: [0-9|\.]+' | grep -oP '[0-9]+(\.[0-9]+)(\.[0-9]+)') && \
#     sudo mkdir -pm755 /etc/vulkan/icd.d/ && \
#     echo "{\
#     \"file_format_version\" : \"1.0.0\",\
#     \"ICD\": {\
#         \"library_path\": \"libGLX_nvidia.so.0\",\
#         \"api_version\" : \"${VULKAN_API_VERSION}\"\
#     }}" | sudo tee /etc/vulkan/icd.d/nvidia_icd.json > /dev/null && \
#     sudo mkdir -pm755 /usr/share/glvnd/egl_vendor.d/ && \
#     echo "{\
#     \"file_format_version\" : \"1.0.0\",\
#     \"ICD\": {\
#         \"library_path\": \"libEGL_nvidia.so.0\"\
#     }}" | sudo tee /usr/share/glvnd/egl_vendor.d/10_nvidia.json > /dev/null

# Build and install Realsense-SDK
WORKDIR /home/$USERNAME
RUN git clone --depth=1 --progress https://github.com/IntelRealSense/librealsense.git --branch v2.56.2
WORKDIR /home/$USERNAME/librealsense
RUN sudo mkdir -p /etc/udev/rules.d && \
    ./scripts/setup_udev_rules.sh \
    ./scripts/patch-realsense-ubuntu-lts-hwe.sh 
RUN mkdir build && cd build \
    && cmake ../ \
    && sudo make uninstall -j4 && make clean && make -j4 && sudo make install -j4

# Build and install Livox-SDK2
WORKDIR /home/$USERNAME
RUN git clone --depth=1 --progress https://github.com/Smart-Wheelchair-RRC/Livox-SDK2.git 
RUN cd Livox-SDK2 \
    && mkdir build \
    && cd build \
    && cmake .. && make -j \
    && sudo make install -j4

RUN git clone --depth=1 https://github.com/koide3/small_gicp.git
RUN cd small_gicp \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j \
    && sudo make install

RUN curl -s --compressed "https://koide3.github.io/ppa/ubuntu2204/KEY.gpg" | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/koide3_ppa.gpg >/dev/null  \
    && echo "deb [signed-by=/etc/apt/trusted.gpg.d/koide3_ppa.gpg] https://koide3.github.io/ppa/ubuntu2204 ./" | sudo tee /etc/apt/sources.list.d/koide3_ppa.list \
    && sudo apt update && sudo apt install -y libiridescence-dev libboost-all-dev libglfw3-dev libmetis-dev libgtsam-points-cuda12.2-dev \
    && sudo apt install -y ros-humble-glim-ros-cuda12.2

COPY rosPkgs.list /tmp/rosPkgs.list
RUN sudo apt-get update \
    && sudo apt-get -y install --no-install-recommends $(cat /tmp/rosPkgs.list) \
    && sudo rm -rf /var/lib/apt/lists/*