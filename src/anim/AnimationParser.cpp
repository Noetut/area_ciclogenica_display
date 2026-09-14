#include "AnimationParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "util/StringUtil.h"

namespace {

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string ToUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return str;
}

bool IsDigitString(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// Strip inline comments starting with '#' or '//'
std::string StripComments(const std::string& line) {
    size_t hashPos = line.find('#');
    size_t slashPos = line.find("//");
    size_t pos = std::min(hashPos, slashPos);
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

// Try parsing duration string like "300ms", "0.5s", "300", "@250ms"
bool TryParseDuration(std::string token, double& outSeconds) {
    token = Trim(token);
    if (token.empty()) return false;
    if (token[0] == '@') token = token.substr(1);

    std::string upper = ToUpper(token);
    if (upper.size() > 2 && upper.substr(upper.size() - 2) == "MS") {
        try {
            double ms = std::stod(token.substr(0, token.size() - 2));
            outSeconds = ms / 1000.0;
            return true;
        } catch (...) {
            return false;
        }
    }

    if (upper.size() > 1 && upper.back() == 'S') {
        try {
            double s = std::stod(token.substr(0, token.size() - 1));
            outSeconds = s;
            return true;
        } catch (...) {
            return false;
        }
    }

    // Pure number defaults to milliseconds
    try {
        size_t idx = 0;
        double val = std::stod(token, &idx);
        if (idx == token.size()) {
            outSeconds = val / 1000.0;
            return true;
        }
    } catch (...) {
    }

    return false;
}

bool FileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring GetExecutableDir() {
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) return L".";
    for (int i = static_cast<int>(len) - 1; i >= 0; --i) {
        if (buffer[i] == L'\\' || buffer[i] == L'/') {
            buffer[i] = L'\0';
            break;
        }
    }
    return buffer;
}

} // namespace

std::string AnimationParser::ResolvePath(const std::string& inputPath) {
    if (inputPath.empty()) {
        const char* defaults[] = {
            "animations/strobe_show.txt",
            "animations/sequential_wave.txt",
            "../animations/strobe_show.txt",
            "../animations/sequential_wave.txt",
            "../../animations/strobe_show.txt",
            "../../animations/sequential_wave.txt"
        };
        for (const char* candidate : defaults) {
            std::wstring wide = AnsiToWide(candidate);
            if (FileExists(wide)) return candidate;
        }

        std::wstring exeDir = GetExecutableDir();
        std::wstring cand1 = exeDir + L"\\animations\\strobe_show.txt";
        if (FileExists(cand1)) return WideToUtf8(cand1);
        std::wstring cand2 = exeDir + L"\\animations\\sequential_wave.txt";
        if (FileExists(cand2)) return WideToUtf8(cand2);
        std::wstring cand3 = exeDir + L"\\..\\animations\\strobe_show.txt";
        if (FileExists(cand3)) return WideToUtf8(cand3);

        return "animations/strobe_show.txt";
    }

    std::wstring direct = AnsiToWide(inputPath);
    if (FileExists(direct)) return inputPath;

    // Check with animations/ prefix
    std::string animPrefix = "animations/" + inputPath;
    if (FileExists(AnsiToWide(animPrefix))) return animPrefix;

    std::string upAnimPrefix = "../animations/" + inputPath;
    if (FileExists(AnsiToWide(upAnimPrefix))) return upAnimPrefix;

    std::wstring exeDir = GetExecutableDir();
    std::wstring candExe = exeDir + L"\\" + direct;
    if (FileExists(candExe)) return WideToUtf8(candExe);

    std::wstring candExeAnim = exeDir + L"\\animations\\" + direct;
    if (FileExists(candExeAnim)) return WideToUtf8(candExeAnim);

    return inputPath;
}

bool AnimationParser::LoadFromFile(const std::string& path, AnimationSequence& outSequence, std::string& outError) {
    std::wstring widePath = AnsiToWide(path);
    HANDLE hFile = CreateFileW(widePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        outError = "Cannot open file: " + path;
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        outError = "Failed to query size for: " + path;
        return false;
    }

    std::string buffer(fileSize, '\0');
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, &buffer[0], fileSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        outError = "Failed to read file: " + path;
        return false;
    }
    CloseHandle(hFile);
    buffer.resize(bytesRead);

    return ParseString(buffer, outSequence, outError);
}

bool AnimationParser::ParseString(const std::string& content, AnimationSequence& outSequence, std::string& outError) {
    outSequence.frames.clear();
    outSequence.name = "Untitled";
    outSequence.loop = true;
    outSequence.defaultStep = 0.3;

    std::istringstream stream(content);
    std::string rawLine;
    int lineNumber = 0;

    while (std::getline(stream, rawLine)) {
        ++lineNumber;
        std::string line = Trim(StripComments(rawLine));
        if (line.empty()) continue;

        // Check for headers (key: value or key = value)
        size_t colonPos = line.find(':');
        size_t equalPos = line.find('=');
        size_t sepPos = (colonPos != std::string::npos) ? colonPos : equalPos;

        if (sepPos != std::string::npos) {
            std::string key = ToUpper(Trim(line.substr(0, sepPos)));
            std::string val = Trim(line.substr(sepPos + 1));

            if (key == "LOOP") {
                std::string valUpper = ToUpper(val);
                outSequence.loop = (valUpper == "TRUE" || valUpper == "1" || valUpper == "YES");
                continue;
            } else if (key == "DEFAULT_STEP" || key == "STEP" || key == "DEFAULT_DURATION") {
                double stepSec = 0.3;
                if (TryParseDuration(val, stepSec)) {
                    outSequence.defaultStep = stepSec;
                }
                continue;
            } else if (key == "NAME") {
                outSequence.name = val;
                continue;
            }
        }

        // Frame line format: [duration] ACTION1 [& ACTION2 ...]
        std::istringstream lineStream(line);
        std::string firstToken;
        lineStream >> firstToken;

        double frameDuration = outSequence.defaultStep;
        bool hasDuration = false;

        double parsedDuration = 0.0;
        if (TryParseDuration(firstToken, parsedDuration)) {
            frameDuration = parsedDuration;
            hasDuration = true;
        }

        // Collect remaining tokens on this line
        std::string commandPart;
        if (hasDuration) {
            std::getline(lineStream, commandPart);
        } else {
            // First token was part of the command
            std::string rest;
            std::getline(lineStream, rest);
            commandPart = firstToken + " " + rest;
        }
        commandPart = Trim(commandPart);
        if (commandPart.empty()) {
            // Just a delay / wait frame
            AnimationFrame waitFrame;
            waitFrame.duration = frameDuration;
            outSequence.frames.push_back(waitFrame);
            continue;
        }

        // Split multiple actions separated by '&'
        std::vector<std::string> subCommands;
        {
            std::istringstream cmdStream(commandPart);
            std::string subCmd;
            while (std::getline(cmdStream, subCmd, '&')) {
                subCmd = Trim(subCmd);
                if (!subCmd.empty()) {
                    subCommands.push_back(subCmd);
                }
            }
        }

        AnimationFrame frame;
        frame.duration = frameDuration;

        for (const auto& cmdStr : subCommands) {
            std::istringstream cs(cmdStr);
            std::string verb;
            cs >> verb;
            std::string verbUpper = ToUpper(verb);

            if (verbUpper == "ALL_ON" || verbUpper == "ALLON") {
                AnimationAction action;
                action.type = ActionType::AllOn;
                frame.actions.push_back(action);
            } else if (verbUpper == "ALL_OFF" || verbUpper == "ALLOFF" || verbUpper == "CLEAR") {
                AnimationAction action;
                action.type = ActionType::AllOff;
                frame.actions.push_back(action);
            } else if (verbUpper == "ON") {
                std::string arg;
                cs >> arg;
                std::string argUpper = ToUpper(arg);
                if (argUpper == "ALL") {
                    AnimationAction action;
                    action.type = ActionType::AllOn;
                    frame.actions.push_back(action);
                } else if (IsDigitString(arg)) {
                    AnimationAction action;
                    action.type = ActionType::TurnOn;
                    // In scripts, user specifies 1-based index (matching UI/keyboard [1]-[9])
                    action.targetIndex = std::stoi(arg) - 1;
                    frame.actions.push_back(action);
                } else if (!arg.empty()) {
                    AnimationAction action;
                    action.type = ActionType::TurnOn;
                    action.targetName = arg;
                    frame.actions.push_back(action);
                }
            } else if (verbUpper == "OFF") {
                std::string arg;
                cs >> arg;
                std::string argUpper = ToUpper(arg);
                if (argUpper == "ALL") {
                    AnimationAction action;
                    action.type = ActionType::AllOff;
                    frame.actions.push_back(action);
                } else if (IsDigitString(arg)) {
                    AnimationAction action;
                    action.type = ActionType::TurnOff;
                    action.targetIndex = std::stoi(arg) - 1;
                    frame.actions.push_back(action);
                } else if (!arg.empty()) {
                    AnimationAction action;
                    action.type = ActionType::TurnOff;
                    action.targetName = arg;
                    frame.actions.push_back(action);
                }
            } else if (verbUpper == "TOGGLE") {
                std::string arg;
                cs >> arg;
                if (IsDigitString(arg)) {
                    AnimationAction action;
                    action.type = ActionType::Toggle;
                    action.targetIndex = std::stoi(arg) - 1;
                    frame.actions.push_back(action);
                } else if (!arg.empty()) {
                    AnimationAction action;
                    action.type = ActionType::Toggle;
                    action.targetName = arg;
                    frame.actions.push_back(action);
                }
            } else if (verbUpper == "MASK") {
                std::string maskStr;
                cs >> maskStr;
                AnimationAction action;
                action.type = ActionType::SetMask;
                for (char c : maskStr) {
                    action.mask.push_back(c == '1');
                }
                frame.actions.push_back(action);
            } else if (verbUpper == "WAIT" || verbUpper == "PAUSE" || verbUpper == "SLEEP") {
                // Just delay, no action needed
            } else {
                // Unknown command
                outError = "Line " + std::to_string(lineNumber) + ": Unknown command '" + verb + "'";
                return false;
            }
        }

        outSequence.frames.push_back(frame);
    }

    if (outSequence.frames.empty()) {
        outError = "Animation file contains no frames.";
        return false;
    }

    return true;
}
