FROM archlinux/base

RUN cd ~;\
    pacman -Syu --noconfirm;\
    pacman -S base-devel sudo git --noconfirm;\
    useradd shitty_makepkg -m;\
    passwd -d shitty_makepkg;\
    echo "shitty_makepkg ALL=(ALL) ALL" >> /etc/sudoers;\
    sudo -u shitty_makepkg bash -c 'cd ~; git clone https://aur.archlinux.org/cpprestsdk.git; cd cpprestsdk; makepkg -sic --noconfirm';\
    userdel shitty_makepkg --remove;\
    mkdir app;

WORKDIR /root/app
COPY ./bin/kap_ssd_lab_2 /root/app/

EXPOSE 80

ENTRYPOINT [ "/root/app/kap_ssd_lab_2", "1eafb85749233507d5dbf83c9d14bb0a" ]