FROM gitlab.ika.rwth-aachen.de:5050/fb-fi/its-modules/communication/mqtt-client-interfaces:latest_traffic-twin-tracing_ci

WORKDIR /ws

COPY mqtt_client /ws/src/mqtt_client
RUN apt-get update && rosdep update && rosdep install -y --from-paths /ws/src --ignore-src --rosdistro $ROS_DISTRO
RUN . /opt/ws_base_image/install/setup.bash && colcon build --symlink-install && source install/setup.bash