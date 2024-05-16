#include <map>
/*
    vị trí của của các item tương ứng với vị trí file âm thanh tên lớp học
    file âm thanh được lưu ở thư mục "01" trên thẻ sd
*/

// create list with item as key is ClassCode and value is ClassName

std::map<String, String> ClassCodeAudioMap = {
    {"1", "Lap trinh web"},
    {"2", "Lap trinh di dong"},
    {"3", "Lap trinh nhung"},
    {"4", "Lap trinh game"},
    {"5", "Lap trinh ung dung"},
};

/*
    vị trí của của các item tương ứng với vị trí file âm thanh tên sinh viên
    file âm thanh được lưu ở thư mục "02" trên thẻ sd
*/

std::map<String, String> StudentCodeAudioMap = {
    {"N20DCPT009", "Luong Quoc Dien"},
    {"N20DCPT021", "Do Hung Hao"},
    {"N20DCPT044", "Tran Van Ngan"},
};
