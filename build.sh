if [ ! -d extra ] && [ -e extra.yml ]; then
  vita-libs-gen -c extra.yml extra
  cd extra
  cmake .
  make
  cd ..
fi

if [ ! -d build ]; then
  mkdir build
fi

cd build
cmake ../
make
cd ..