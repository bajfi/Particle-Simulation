NAME = particle_system
SRC = *.cpp
OBJ = $(SRC:.cpp=.o)

# Remove Mac-specific frameworks and add Linux libraries
LIBS = -lGL -lGLEW -lglfw -lOpenCL -lEGL -lX11

# Update include paths for Linux and add OpenCL target version
INCLUDES = -I/usr/include/CL
CXXFLAGS = -std=c++14 -DCL_TARGET_OPENCL_VERSION=300

RED = "\033[1;38;2;225;20;20m"
ORANGE = "\033[1;38;2;255;120;10m"
YELLO = "\033[1;38;2;255;200;0m"
GREEN = "\033[1;38;2;0;170;101m"
LG = "\033[1;38;2;167;244;66m"
BLUE = "\033[1;38;2;50;150;250m"
PURPLE = "\033[1;38;2;150;75;255m"
WHITE = "\033[1;38;2;255;250;232m"

all: $(NAME)

$(NAME): $(SRC)
	@echo $(YELLO)Making particle_system
	@g++ -O3 $(CXXFLAGS) $(SRC) -o $(NAME) $(INCLUDES) $(LIBS)
	@echo $(GREEN)Done!

clean:
	@echo $(YELLO)Cleaning o files
	@/bin/rm -f $(OBJ)

fclean: clean
	@echo $(YELLO)Removing excutable
	@rm -f $(NAME)

re:	fclean all