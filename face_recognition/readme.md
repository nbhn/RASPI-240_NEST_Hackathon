run:

pip install -r requirements.txt

# dlib challenges

Note that dlib (which is a dependency of face_recognition) can sometimes be challenging to install directly, especially on Windows. If you encounter issues, you might need to install additional system dependencies:

# for Ubuntu / Debian:

sudo apt-get update
sudo apt-get install -y build-essential cmake
sudo apt-get install -y libopenblas-dev liblapack-dev
sudo apt-get install -y python3-dev
pip install -r requirements.txt

# for macOS:

brew install cmake
pip install -r requirements.txt

# for Windows:

pip install https://github.com/jloh02/dlib/releases/download/v19.22/dlib-19.22.99-cp39-cp39-win_amd64.whl
pip install -r requirements.txt