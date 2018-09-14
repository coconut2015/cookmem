# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/xenial64"

  # Be sure to have the following vagrant plugin installed.
  # vagrant plugin install vagrant-vbguest
  config.vm.synced_folder ".", "/vagrant"

  config.vm.provider "virtualbox" do |vb|
    vb.name = "cookmem"
    vb.gui = false
    vb.memory = "512"
  end

  config.vm.define "cookmem" do |cookmem|
    cookmem.vm.hostname = "cookmem"
    cookmem.vm.network "private_network", ip: "10.2.3.12"
    cookmem.vm.provision "shell", path: "vmsetup/install.sh"
    # cookmem
    cookmem.vm.network "forwarded_port", guest: 6543, host: 6543
  end

  config.ssh.forward_x11 = true
end
