# Define directories
incdir := ./include
srcdir := ./src
objdir := ./objs
libdir := ./lib
bindir := ./bin

# Create directories if they do not exist
createdirs := $(shell for dir in $(incdir) $(srcdir) $(objdir) $(libdir) $(bindir); \
  do \
    mkdir -p $$dir; \
  done)

# Define compiler and flags
CXX := g++
CXXFLAGS := -std=c++11 -O3 -fPIC -g $(shell root-config --cflags)
CPPFLAGS := -I./ -I$(incdir) -I$(shell root-config --incdir) 
LDFLAGS := $(shell root-config --libs) -fopenmp 

# Define main application binary and source files
main_app := $(bindir)/gainMatch
cpp_srcfiles := $(shell find $(srcdir) -name "*.cpp")
cpp_objects := $(patsubst $(srcdir)/%.cpp, $(objdir)/%.o, $(cpp_srcfiles))

objects := $(cpp_objects) 

# Define ROOT dictionary files, library, and object files
dict := $(srcdir)/sps_dict.cxx
dict_inputs := $(incdir)/DataStructs.h $(incdir)/GainMap.h $(incdir)/ChannelMap.h $(incdir)/LinkDef_sps.h
dictlib := $(libdir)/libSPSDict
dictobj := $(objdir)/sps_dict.o

# Default target: build the main application
all: main_app shared_lib

# Build the main application, depending on the object files and shared library
main_app: $(objects) shared_lib 
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $(main_app) $(objects) -L$(libdir) -lSPSDict $(LDFLAGS)

# Compile source files into object files
$(objdir)/%.o: $(srcdir)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $^

# Rule for building the shared library
shared_lib: $(dictlib).so

# Compile the ROOT dictionary object file and create a shared library
$(dictlib).so: $(dictobj)
	$(CXX) $(CXXFLAGS) $^ -shared -o $@ $(LDFLAGS)
	cp $(srcdir)/*.pcm $(libdir)

$(dictobj): $(dict)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $^

# Generate the ROOT dictionary source file
$(dict) : $(dict_inputs)
	rootcint -f $(dict) $(dict_inputs)

# Clean up generated files
clean:
	rm -f $(dict) \
		  $(dictobj) \
		  $(dictlib).so \
		  $(libdir)/*.pcm \
		  $(srcdir)/*.pcm \
		  $(objects) \
		  $(main_app)
