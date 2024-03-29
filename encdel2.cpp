
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <array> 
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif


struct VideoInfo
{
    double total_duration;
    unsigned long int total_size;
    int total_bit_rate;
    std::pair<int, int> resolution;
    std::string aspect_ratio;
};

bool isValidNumber(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

bool file_exists(const std::string& filepath) {
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}

#ifdef _WIN32
    std::string run_command(const std::string& command)
    {
        std::string cmd = command + " 2>&1";
        FILE* fp = _popen(cmd.c_str(), "rb");
        if (fp == nullptr)
        {
            throw std::runtime_error("popen error: " + std::string(strerror(errno)));
        }
        
        std::array<char, 256> buffer;
        
        std::string result;
        while (fgets(buffer.data(), 256, fp) != nullptr)
        {
            result += buffer.data();
        }
    
        int exit_code = _pclose(fp);
        
        if (exit_code == 0)
        {
            return result;
        }
        else
        {
            throw std::runtime_error("Command error: exit code = " + std::to_string(exit_code));
        }
    }
#else
    std::string run_command(const std::string& command)
    {
        std::string cmd = command + " 2>&1";
        FILE* fp = popen(cmd.c_str(), "r");
        if (fp == nullptr)
        {
            throw std::runtime_error("popen error: " + std::string(strerror(errno)));
        }
        
        std::array<char, 256> buffer;
        
        std::string result;
        while (fgets(buffer.data(), 256, fp) != nullptr)
        {
            result += buffer.data();
        }
    
        int exit_code = pclose(fp);
        
        if (WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0)
        {
            return result;
        }
        else
        {
            throw std::runtime_error("Command error: exit code = " + std::to_string(WEXITSTATUS(exit_code)));
        }
    }
#endif

std::string get_aspect_ratio(const double aspect_ratio)
{
    int approx_ratio = static_cast<int>(std::round(aspect_ratio));
    switch (approx_ratio)
    {
        case 1:
            return "1:1";
        case 4:
            return "4:3";
        case 5:
            return "5:4";
        case 16:
            return "16:9";
        case 17:
            return "17:10";
        case 21:
            return "21:9";
        default:
            return std::to_string(approx_ratio);
    }
}

VideoInfo get_video_info(const std::string& input)
{
    std::string command = "ffprobe -v error -show_entries format=size,duration,bit_rate:stream=codec_type,width,height,bit_rate,display_aspect_ratio -of default=noprint_wrappers=1 -i \"" + input + "\"";
    std::istringstream ffprobe_pipe(run_command(command));
    
    std::unordered_map<std::string, std::string> info_map;
    std::string line;
    while (std::getline(ffprobe_pipe, line))
    {
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (value != "N/A")
            {
                info_map[key] = value;
            }
        }
    }

    VideoInfo info;
    info.total_duration = std::stod(info_map["duration"]);
	/*if (isValidNumber(info_map["size"]))
	{
		long double result;// = std::stoi(info_map["size"]);
    // ... используйте результат, если строки валидны
	}
	else
	{
    // обработка ошибки: ваша строка не является корректным числом
		throw std::invalid_argument("Invalid argument: size " + info_map["size"]);
	}*/
    //info.total_size = std::stoi(info_map["size"]);
    info.total_bit_rate = std::stoi(info_map["bit_rate"]);
    info.resolution = std::make_pair(std::stoi(info_map["width"]), std::stoi(info_map["height"]));
    double dar = std::stod(info_map["display_aspect_ratio"]);
    info.aspect_ratio = get_aspect_ratio(dar);
    
    return info;
}

std::string format_duration(double duration) 
{
    int hours = static_cast<int>(duration / 3600);
    duration -= hours * 3600;
    int minutes = static_cast<int>(duration / 60);
    duration -= minutes * 60;
    int seconds = static_cast<int>(duration);

    std::ostringstream formatted_duration;
    formatted_duration << std::setfill('0') << std::setw(2) << hours << ":"
                       << std::setw(2) << minutes << ":"
                       << std::setw(2) << seconds;

    return formatted_duration.str();
}

void set_input_file(const std::string& input_path)
{
    VideoInfo video_info = get_video_info(input_path);
	
	std::string formatted_duration = format_duration(video_info.total_duration);

    std::cout << "Duration: " << formatted_duration << " " << std::endl;
    //std::cout << "Size: " << video_info.total_size << " byte" << std::endl;
    std::cout << "Total bitrate: " << (ceil(video_info.total_bit_rate/(1000*100))/10) << " Mbps" << std::endl;
    std::cout << "Resolution: " << video_info.resolution.first << "x" << video_info.resolution.second << std::endl;
    std::cout << "Aspect ratio: " << video_info.aspect_ratio << std::endl;

}



/*
if (isValidNumber(your_string))
{
    int result = std::stoi(your_string);
    // ... используйте результат, если строки валидны
}
else
{
    // обработка ошибки: ваша строка не является корректным числом
    throw std::invalid_argument("Invalid argument: " + your_string);
}
*/



std::vector<std::string> valid_extensions = {".wmv", ".avi", ".asf", ".flv", ".mkv", ".m4v", ".VOB"};


std::string format_ffmpeg(const std::string& input_path)
{
	VideoInfo video_info = get_video_info(input_path);
	std::ostringstream formatted_ffmpeg;
   
	if((video_info.resolution.second >= 1080) && (video_info.resolution.first > 1280))
	{
		formatted_ffmpeg << std::setfill('0') << std::setw(1) << " -b:v "; 
		if(((ceil(video_info.total_bit_rate/(1000*100))/10) > 8.1) || ((ceil(video_info.total_bit_rate/(1000*100))/10) < 5))
		{
			 formatted_ffmpeg << std::setfill('0') << std::setw(1) << "8M ";		
		}
		else
		{
			formatted_ffmpeg << std::setfill('0') << std::setw(2) << (ceil(video_info.total_bit_rate/(1000*100))/10) << "M ";
		}
		formatted_ffmpeg << std::setfill('0') << std::setw(1) <<  " -s 1920x1080"; 
	}
	else if ((video_info.resolution.first == 1280)&& (video_info.resolution.second == 720))
	{
		formatted_ffmpeg << std::setfill('0') << std::setw(1) << " -b:v "; 
		if((ceil(video_info.total_bit_rate/(1000*100))/10) > 5.3)
		{
			 formatted_ffmpeg << std::setfill('0') << std::setw(1) << "5M ";		
		}
		else
		{
			formatted_ffmpeg << std::setfill('0') << std::setw(2) << (ceil(video_info.total_bit_rate/(1000*100))/10) << "M ";
		}
		formatted_ffmpeg << std::setfill('0') << std::setw(1) <<  " -s 1280x720"; 
		
	}
	else
	{
		formatted_ffmpeg << std::setfill('0') << std::setw(1) << " -b:v 4.5M -vf yadif -preset slow "; 
	}
	
    //formatted_ffmpeg << std::setfill('0') << std::setw(2) << s;

    return formatted_ffmpeg.str();
}


std::string slash_replace(const std::string& input_path)
{
	std::string path = input_path;
    std::cout << "Original path: " << path << std::endl;
    
    // Find and replace all slashes with backslashes
    size_t pos = 0;
    while ((pos = path.find('/', pos)) != std::string::npos) {
        path.replace(pos, 1, "\\");
        pos += 1;
    }
    
    std::cout << "Updated path: " << path << std::endl;
    
    return path;
}

void convert_video(const std::string& input_path)
{
    const auto last_dot_index = input_path.find_last_of('.');
    std::string output_path = input_path.substr(0, last_dot_index) + ".mp4";
	
	std::string command = "ffmpeg -i \"" + input_path; 
	command += "\" -map_metadata -1 -map_chapters -1 -avoid_negative_ts make_zero -c:v libx264 ";
	command += format_ffmpeg(input_path) + " -r 29.97 -c:a ";
#ifdef _WIN32
	command += "libvo_aacenc";
#else
	command += "aac";
#endif
	command += " -b:a 320K -n \""+ output_path + "\"";
    std::cout << "command: " << command << std::endl;
	std::cout << std::endl;
	
	system(command.c_str());

	// Удалит входной файл после обработки
	std::string command_rm_input_path = "";
#ifdef _WIN32
    system("timeout /t 2");
	command_rm_input_path = "DEL /Q /F \"" + slash_replace(input_path) +"\"";
#else
    system("sleep 2");
	command_rm_input_path = "rm -y \"" + input_path + "\"";
#endif
	system(command_rm_input_path.c_str());
}

// Включайте все заголовки, как указано в вашем исходном коде

// Используйте функции: run_command, get_aspect_ratio, get_video_info,
// set_input_file, format_duration, format_ffmpeg, convert_video, как и в вашем исходном коде


bool is_directory(const std::string& directory_path, dirent* entry)
{
#ifdef _DIRENT_HAVE_D_TYPE
    return entry->d_type == DT_DIR;
#else
    struct stat path_stat;
    std::string full_path = directory_path + "/" + entry->d_name;
    if (stat(full_path.c_str(), &path_stat) == 0)
    {
        return S_ISDIR(path_stat.st_mode);
    }
    else
    {
        return false;
    }
#endif
}


void traverse_directory(const std::string& directory_path)
{
    DIR* dir;
    dirent* entry;

    if ((dir = opendir(directory_path.c_str())) != nullptr)
    {
        while ((entry = readdir(dir)) != nullptr)
        {
            if (is_directory(directory_path, entry))
            {
                std::string sub_directory = entry->d_name;

                if (sub_directory != "." && sub_directory != "..")
                {
                    traverse_directory(directory_path + "/" + sub_directory);
                }
            }
            else
            {
                const std::string entry_name = entry->d_name;
                const auto ext_start = entry_name.find_last_of('.');

                if (ext_start != std::string::npos)
                {
                    const std::string ext = entry_name.substr(ext_start);

                    if (std::find(valid_extensions.begin(), valid_extensions.end(), std::string(ext)) != valid_extensions.end())
                    {
                        std::string file_path = directory_path + "/" + entry_name;

                        std::cout << "Converting video file: " << file_path << std::endl;
                        set_input_file(file_path);
                        convert_video(file_path);
                    }
                }
            }
        }
        closedir(dir);
    }
}


int main()
{
    std::string start_directory = ".";  // Задайте стартовый каталог (или измените на нужный)
    traverse_directory(start_directory);

    return 0;
}

