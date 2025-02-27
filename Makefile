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
OBJ_DIR = obj/

SRC = $(SRC_DIR)/main.cpp \
	  $(UTILS_DIR)/Server.cpp \
	  $(UTILS_DIR)/ServerUtils.cpp \
	  $(UTILS_DIR)/ServerInit.cpp \
	  $(UTILS_DIR)/ServerPost.cpp \
	  $(UTILS_DIR)/ServerCGI.cpp \
	  $(UTILS_DIR)/ServerGet.cpp \
	  $(UTILS_DIR)/ServerFolders.cpp \
	  $(UTILS_DIR)/ServerDelete.cpp \
	  $(UTILS_DIR)/ServerErrors.cpp \
	  $(UTILS_DIR)/ResolvePaths.cpp \
	  $(UTILS_DIR)/Config.cpp \
	  $(UTILS_DIR)/ServerConfig.cpp \
	  $(UTILS_DIR)/LocationConfig.cpp \
	  $(UTILS_DIR)/HttpRequestParser.cpp \
	  $(UTILS_DIR)/FDStreamBuf.cpp \
	  $(UTILS_DIR)/SignalHandler.cpp \

OBJ = $(SRC:$(UTILS_DIR)%.cpp=$(OBJ_DIR)%.o)

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(PASTEL_BLUE)Compiling sources... ⏳$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)
	@echo "$(PASTEL_GREEN)Build complete! 🎉 Executable created: $(NAME)$(RESET)"

$(OBJ_DIR)%.o: $(UTILS_DIR)%.cpp
	@echo "$(PASTEL_YELLOW)Compiling $<...$(RESET)"
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(PASTEL_YELLOW)Cleaning object files... ✔️$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(PASTEL_PINK)Cleaning executable... ✅$(RESET)"
	@echo "$(PASTEL_LILAC)All clean! 😊✨$(RESET)"

re: fclean all
	@echo "$(PASTEL_BLUE)Rebuilt everything from scratch! 🔄🚀$(RESET)"
