# yadist-fs
Yet Another Distribution file system on demand

## BUILD

requirement

- cmake (best for 3.14 or newer)
- make (4.0 or newer)
- ninja (1.7 or newer)

build the project

```
 # git clone https://github.com/ixuexi/yadist-fs.git
 # cd yadist-fs
 # mkdir objects
 # cd objects
 # make -j
```

installed dir

yadist-fs/package


## simple test

change dir to `package/dist/bin`

- start a fs server daemon 

```
sudo ./start_ser.sh
```

- start a fs actor daemon

```
sudo ./start_act.sh
```

- test for chroot

```
./chroot_test
```
