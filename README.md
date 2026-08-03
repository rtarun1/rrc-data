# RRC Handheld Data Collection Rig

## Prerequisites

- **Ubuntu** (tested on 22.04)
- **VSCode**
- **Remote Development Extension by Microsoft** (Inside VSCode)
- **Docker Installation**
  ```bash
  # Install Docker using convenience script
  curl -fsSL https://get.docker.com -o get-docker.sh
  sudo sh ./get-docker.sh

  # Post-install configuration
  sudo groupadd docker
  sudo usermod -aG docker $USER

  # Verify if Docker service is enabled
  sudo systemctl is-enabled docker

  # If not enable it
  sudo systemctl enable docker.service
  sudo systemctl enable containerd.service
  ```
>[!IMPORTANT]
>**Reboot before proceeding further**
```bash
  # GHCR Authentication
  echo "<YOUR_GITHUB_PAT>" | docker login ghcr.io -u <YOUR_GITHUB_USERID> --password-stdin
  ```

### Enabling GPU Support 

- [**Install the NVIDIA Container Toolkit**](http://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)
- **Enabling Nvidia GPU for simulation**

  | Hardware | Requirement  |
  | :------- | :----------- |
  | GPU      | CUDA-enabled |

  | Software      | Requirement                                                           |
  | :------------ | :-------------------------------------------------------------------- |
  | Nvidia Driver | - Ubuntu 22.04 `>=515.43.04` <br/> - Ubuntu 24.04 `>=555.42.02` <br/> |

## Hardware Setup
Information about the Hardware Setup:

**RGB Camera: Realsense D455**
- No Additional Setup is required for getting data, just plug-in the type-c cable

**3D LiDAR: Livox MID-360**
 1.  Create new network profile **"Livox"**
 2.  Configure static IP:
      - **IP Address:** 192.168.1.50
      - **Netmask:** 255.255.255.0

## How to Use
- **Set ROS DOMAIN ID** to a random number between 0 to 101 inside the **devcontainer.json** file. Please make sure to read the [Docs.](https://docs.ros.org/en/eloquent/Tutorials/Configuring-ROS2-Environment.html#the-ros-domain-id-variable)
```json
{
  "containerEnv": {
    "ROS_DOMAIN_ID": "27"  // Change this to your unique ID between (0 - 101)
  }
}
```
- **Enter the container**
    - Open Command Pallete with `Ctrl+Shift+P`
    - Select **Dev Containers: Rebuild and Reopen in Container**
    - Choose your development container:

      | Develeopment Container | System requirement         | Use case                        |
      |------------------------|----------------------------|---------------------------------|
      | AMD64/x86 with GPU     | x86 system with Nvidia GPU | Running GLIM GPU                |
      | AMD64/x86 without GPU  | x86 system                 | Running on the NUC rig          |

- **Build workspace and source**
  ```bash
  colcon build --symlink-install && source install/setup.bash
  ```
- **Run the data collection on the NUC rig**
  ```bash
  ros2 launch rrc_data data_collection.launch.py
  ```
- **Save the synced data**
  ```bash
  ros2 launch rrc_data save_synced_data.launch.py
  ```
- **Save the synced data with GLIM poses**
  ```bash
  ros2 launch rrc_data save_synced_data_wglim.launch.py
  ```