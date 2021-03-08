#!/bin/bash
install_location="/NIR_Imaging" 
prebuilt_image="/NIR_Imaging/docker_image"

if [ -d "$install_location" ]
then 
	rm -r $install_location
	mkdir $install_location
	echo "$install_location is being wiped"
else 
	mkdir $install_location
	echo "$install_location created"
fi 

cp -r * $install_location 

if [ "$1" = "docker" ] 
then 

	chmod +x /NIR_Imaging/Dependencies/install_pylon.sh
	./NIR_Imaging/Dependencies/install_pylon.sh
	echo "Pylon Installed Locally"
	
	sudo apt-get install -y docker=1.5-1
	
	if [ -r "$prebuilt_image" ]
	then
		echo "Found $prebuilt_image : will not rebuild"
		sudo docker load -i /NIR_Imaging/docker_image -t nely_tool
	else
		echo "No $prebuilt_image found : "
		sudo docker build /NIR_Imaging -t nely_tool
		sudo docker save -o /NIR_Imaging/docker_image
	fi
	
	echo "alias run_nir_imaging=\"docker run --privileged --cap-add=sys_nice --cpu-rt-runtime=990000 
			--ulimit rtprio=99 --rm -it -v /tmp/.X11-unix:/tmp/.X11-unix 
			-v /dev/bus/usb:/dev/bus/usb -v /data:/data
			-e DISPLAY=unix$DISPLAY nely_tool /NIR_Imaging/GUI_BASLER_m\"" >> ~/.bashrc
	echo "alias run_pylon=\"/opt/pylon5/bin/PylonViewerApp\""   >> ~/.bashrc
	source ~/.bashrc

elif [ "$1" = "local" ]
then 
	
	chmod +x /NIR_Imaging/setup_local.sh 
	/NIR_Imaging/setup_local.sh 

	echo "alias run_nir_imaging=\"/NIR_Imaging/GUI_BASLER_m\""        >> ~/.bashrc
	echo "alias run_pylon=\"sudo /opt/pylon5/bin/PylonViewerApp\"" 	  >> ~/.bashrc
	echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/pylon5/lib64/" >> ~/.bashrc
	
	# Configure Linux Kernel
	echo "* - nofile 50000"  >  /etc/security/limits.conf
	echo "* - rtprio 99"     >> /etc/security/limits.conf
	
	# Configure IO Scheduler 
	 
	source ~/.bashrc

else 
	echo "Invalid Parameter: Expecting either Local or Docker"
	exit 1
fi 

# Configure Host Machine Settings 
zcat /proc/config.gz | grep CONFIG_RT_GROUP_SCHED

echo "run_nir_imaging and run_pylon have been successfully aliased"



