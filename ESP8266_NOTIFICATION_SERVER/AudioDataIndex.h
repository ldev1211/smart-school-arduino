#include <map>
/*
    vị trí của của các item tương ứng với vị trí file âm thanh tên lớp học
    file âm thanh được lưu ở thư mục "01" trên thẻ sd
*/

// create list with item as key is ClassCode and value is ClassName

std::map<String, String> ClassCodeAudioMap = {
    {"LH1", "Lap trinh web"},
    {"LH2", "Lap trinh di dong"},
    {"LH3", "Lap trinh nhung"},
    {"LH4", "Lap trinh game"},
    {"LH5", "Lap trinh ung dung"},
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
