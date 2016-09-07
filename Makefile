CXX = g++

PREFIX ?= /usr/local
TARGET = hoshizora
OPENCV_INCLUDE ?= $(PREFIX)/include
OPENCV_LIB ?= $(PREFIX)/lib
LIBPNG_INCLUDE ?= $(PREFIX)/include
LIBPNG_LIB ?= $(PREFIX)/lib
LD_FLAGS += -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lpng

CXX_FLAGS += -std=c++14 -fPIC -I$(OPENCV_INCLUDE) -L$(OPENCV_LIB) -I$(LIBPNG_INCLUDE) -L$(LIBPNG_LIB) $(LD_FLAGS)

$(TARGET) : 
	$(CXX) $(CXX_FLAGS) main.cpp -o $(TARGET)
    
install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin

uninstall :
	rm -f $(PREFIX)/bin/$(TARGET)

clean :
	-rm -f $(TARGET)
