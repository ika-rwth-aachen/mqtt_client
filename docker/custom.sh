# TODO: remove once [https://github.com/ros2/ros2_tracing/issues/211] is solved in released version
# overwrite released ros2_tracing packages with fork to support
# 'message-link instrumentation' and 'dual-session mode' in jazzy
cd /docker-ros/ws
git clone --branch jazzy-ika https://github.com/RaphvK/ros2_tracing.git src/ros2_tracing
rosdep update && rosdep install -y -i --from-paths src
source /opt/ros/${ROS_DISTRO}/setup.bash
colcon build --allow-overriding tracetools --allow-overriding tracetools_launch
rm -r src/ros2_tracing log build
