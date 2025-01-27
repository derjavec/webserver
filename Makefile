NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -IIncludes

RESET = \033[0m
PASTEL_PINK = \033[38;5;218m
PASTEL_BLUE = \033[38;5;153m
PASTEL_YELLOW = \033[38;5;229m
PASTEL_GREEN = \033[38;5;150m
PASTEL_LILAC = \033[38;5;183m

SRC_DIR = .
UTILS_DIR = srcs
INCLUDES_DIR = Includes

SRC = $(SRC_DIR)/main.cpp \
	  $(UTILS_DIR)/Client.cpp \
	  $(UTILS_DIR)/Server.cpp \
	  $(UTILS_DIR)/Config.cpp \
	  $(UTILS_DIR)/ServerConfig.cpp \
	  $(UTILS_DIR)/LocationConfig.cpp \

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(PASTEL_BLUE)Compiling sources... ⏳$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)
	@echo "$(PASTEL_GREEN)Build complete! 🎉 Executable created: $(NAME)$(RESET)"

%.o: %.cpp
	@echo "$(PASTEL_YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ)
	@echo "$(PASTEL_YELLOW)Cleaning object files... ✔️$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(PASTEL_PINK)Cleaning executable... ✅$(RESET)"
	@echo "$(PASTEL_LILAC)All clean! 😊✨$(RESET)"

re: fclean all
	@echo "$(PASTEL_BLUE)Rebuilt everything from scratch! 🔄🚀$(RESET)"