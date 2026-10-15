moorhuhn2_add_test(TARGET moorhuhn2_computer_tests NAME bonus.computer
    SOURCE computer.cpp LIBRARIES moorhuhn2_bonus
    LABELS bonus ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_snake_tests NAME bonus.snake
    SOURCE snake.cpp LIBRARIES moorhuhn2_bonus
    LABELS bonus ui TIMEOUT 15)

moorhuhn2_add_test(TARGET moorhuhn2_making_of_tests NAME bonus.making_of
    SOURCE making_of.cpp LIBRARIES moorhuhn2_bonus
    LABELS bonus ui TIMEOUT 15)
