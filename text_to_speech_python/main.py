# Import the required module for text
# to speech conversion
from gtts import gTTS
import time

# This module is imported so that we can
# play the converted audio
import os

# The text that you want to convert to audio
mytext = "Chào mừng bạn Ngạn!"

# get current folder path
current_path = os.path.dirname(os.path.realpath(__file__))

# Language in which you want to convert
language = "vi"

# create foleder named "01" and "02" '03' '04' if not exist in current path
os.makedirs(current_path + "/01", exist_ok=True)
os.makedirs(current_path + "/02", exist_ok=True)
os.makedirs(current_path + "/03", exist_ok=True)


# write func to convert text to speech and save to file
def text_to_speech(text, lang, file_name):
    myobj = gTTS(text=text, lang=lang, slow=False)
    myobj.save(file_name)
    # save file name as mp3 and format it: filename_time.mp3
    # os.system("start " + file_name)
    # i want to wait until the file finish playing
    # time.sleep(5)


#

#     std::map<String, String> ClassCodeAudioMap = {
#     {"LH1", "Lap trinh web"},
#     {"LH2", "Lap trinh di dong"},
#     {"LH3", "Lap trinh nhung"},
#     {"LH4", "Lap trinh game"},
#     {"LH5", "Lap trinh ung dung"},
# };

#     std::map<String, String> StudentCodeAudioMap = {
#     {"N20DCPT009", "Luong Quoc Dien"},
#     {"N20DCPT021", "Do Hung Hao"},
#     {"N20DCPT044", "Tran Van Ngan"},
# };

classes = [
    " lập trình web",
    " lập trình di động",
    " lập trình nhúng",
    " lập trình game",
    " lập trình ứng dụng",
]


students = [
    "Chào mừng bạn Lương Quốc Diễn đã đến với lớp học ",
    "Chào mừng bạn Đỗ Hùng Hảo đã đến với lớp học ",
    "Chào mừng bạn Trần Văn Ngạn đã đến với lớp học ",
]


print("Start converting text to speech")

count = 0

# Thông báo tên lớp học
for item in classes:
    count += 1

    filename = ""
    if count > 100:
        filename = str(count) + ".mp3"
    elif count > 10:
        filename = "0" + str(count) + ".mp3"
    else:
        filename = "00" + str(count) + ".mp3"

    filepath = current_path + "/01/" + filename

    text_to_speech(item, language, filepath)

# thông báo chào mừng sinh viên
count = 0
for item in students:
    count += 1

    filename = ""
    if count > 100:
        filename = str(count) + ".mp3"
    elif count > 10:
        filename = "0" + str(count) + ".mp3"
    else:
        filename = "00" + str(count) + ".mp3"

    filepath = current_path + "/02/" + filename

    text_to_speech(item, language, filepath)

# THÔNG BÁO ĐIỂM DANH LỖI
text_to_speech(
    "Xin lỗi! Hệ thống không thể nhận diện ra bạn!",
    language,
    current_path + "/03/001.mp3",
)
text_to_speech(
    "Xin lỗi! Đã quá giờ điểm danh mất rồi!", language, current_path + "/03/002.mp3"
)
text_to_speech(
    "Xin lỗi! Chưa tới giờ điểm danh bạn ơi!", language, current_path + "/03/003.mp3"
)


print("Finish converting text to speech")
