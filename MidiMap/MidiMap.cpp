#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <RtMidi/RtMidi.h>
#include <vector>
#include <fstream>
#include <windows.h>
#include <optional>
#include <map>

// Array to store the key mappings
std::vector<char> keyMappings;\
std::map<WORD, bool>shiftMappings;

// Function to load the key mappings from the file
bool loadKeyMappings(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open the file: " << filename << std::endl;
        return false;
    }

    char key;
    while (file >> key)
    {
        //if (key == '\n') continue;
        //if (key == '\r') continue;
        keyMappings.push_back(key);
    }

    file.close();
    return true;
}

class FKey
{
public:
    FKey(WORD key, INPUT input, bool useShift)
        : key(key), input(input), useShift(useShift) {}
    WORD key;
    INPUT input;
    bool useShift;
};

// Function to simulate a Shift key press
void pressShift()
{

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;  // No virtual key code
    input.ki.wScan = MapVirtualKeyA(VK_SHIFT, MAPVK_VK_TO_VSC);  // Convert VKID to scan code
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.dwExtraInfo = 0;
    input.ki.time = 0;
    SendInput(1, &input, sizeof(INPUT));  // Send key down event
}

// Function to simulate a Shift key release
void releaseShift()
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;  // No virtual key code
    input.ki.wScan = MapVirtualKeyA(VK_SHIFT, MAPVK_VK_TO_VSC);  // Convert VKID to scan code
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;  // Key up event flag with scan code
    input.ki.dwExtraInfo = 0;
    input.ki.time = 0;
    SendInput(1, &input, sizeof(INPUT));  // Send key down event
}

bool isKeyDown(WORD key) {
    SHORT state = GetAsyncKeyState(key);
    return (state & 0x8000) != 0;
}


// Function to simulate a key press
void pressKey(FKey fkey)
{
    shiftMappings[fkey.key] = fkey.useShift;

    if (isKeyDown(fkey.key))
    {
        //release the key
        INPUT input = INPUT(fkey.input);
        input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;  // Key up event flag with scan code
        SendInput(1, &input, sizeof(INPUT));  // Send key up event
        //releaseKey(fkey);
    }

    if (fkey.useShift)
    {
        pressShift();
    }

    INPUT input = INPUT(fkey.input);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));  // Send key down event

    if (fkey.useShift)
    {
        releaseShift();
    }
}


// Function to simulate a key release
void releaseKey(FKey fkey)
{
    if (shiftMappings[fkey.key] != fkey.useShift)
    {
        return;
    }
    std::cout << "releasing " << (fkey.useShift ? "upper " : "") << "key: " << (char)fkey.key << " with " << (char)MapVirtualKeyA(fkey.input.ki.wScan, MAPVK_VSC_TO_VK) << std::endl;
    INPUT input = INPUT(fkey.input);
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;  // Key up event flag with scan code
    SendInput(1, &input, sizeof(INPUT));  // Send key up event
}


// Function to map MIDI notes to keyboard keys using the loaded array
std::optional<FKey> mapMidiNoteToKey(unsigned char midiNote)
{
    unsigned char midiIndex = midiNote - 36;
    if (midiIndex < keyMappings.size())
    {
        bool useShift = false;
        char keyChar = keyMappings[midiIndex];

        if (isupper(keyChar))
        {
            keyChar = tolower(keyChar);
            useShift = true;
        }
        else if (!islower(keyChar) && !isdigit(keyChar))
        {
            useShift = true;

            switch (keyChar)
            {
            case '!':
                keyChar = '1';
                break;
            case '@':
                keyChar = '2';
                break;
            case '#':
                keyChar = '3';
                break;
            case '$':
                keyChar = '4';
                break;
            case '%':
                keyChar = '5';
                break;
            case '^':
                keyChar = '6';
                break;
            case '&':
                keyChar = '7';
                break;
            case '*':
                keyChar = '8';
                break;
            case '(':
                keyChar = '9';
                break;
            case ')':
                keyChar = '0';
                break;
            default:
                useShift = false;
                break;
            }

        }

        WORD key = VkKeyScan(keyChar);

        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;  // No virtual key code
        input.ki.wScan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);  // Convert VKID to scan code
        input.ki.dwFlags = 0;  // Indicate scan code is being used
        input.ki.dwExtraInfo = 0;
        input.ki.time = 0;

        return std::optional<FKey>{FKey(key, input, useShift)};
        //return ;  // Convert character to virtual key code
    }
    return std::nullopt;
    //return FKey(0, INPUT(), false);  // No mapping found
}

// Callback function to handle incoming MIDI messages
void midiCallback(double deltatime, std::vector<unsigned char>* message, void* userData)
{
    unsigned int nBytes = message->size();

    if (nBytes == 3)
    {
        unsigned char status = message->at(0);
        unsigned char note = message->at(1);
        unsigned char velocity = message->at(2);

        // Filter out unnecessary messages
        if (status == 144 || status == 128)  // 144 (0x90) = Note On, 128 (0x80) = Note Off
        {
            auto opt = mapMidiNoteToKey(note);
            //FKey fkey = mapMidiNoteToKey(note);

            if (opt.has_value())
            {
                if (velocity > 0)
                {
                    //std::cout << "Note On: " << (int)note << " -> Pressing key: " << fkey.key << std::endl;
                    pressKey(opt.value());
                }
                else
                {
                    //std::cout << "Note Off: " << (int)note << " -> Releasing key: " << fkey.key << std::endl;
                    releaseKey(opt.value());
                }
            }
        }
    }
}

int main()
{
    // Load the key mappings from the file
    if (!loadKeyMappings("default.layout"))
    {
        return -1;  // Exit if loading failed
    }

    RtMidiIn* midiin = 0;

    // Initialize RtMidiIn object
    try
    {
        midiin = new RtMidiIn();
    }
    catch (RtMidiError& error)
    {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    // Check available ports
    unsigned int nPorts = midiin->getPortCount();
    if (nPorts == 0)
    {
        std::cout << "No ports available!\n";
        delete midiin;
        return 0;
    }

    // Open the first available port
    midiin->openPort(0);

    // Set the callback function to receive MIDI messages
    midiin->setCallback(&midiCallback);

    // Ignore sysex, timing, and active sensing messages
    midiin->ignoreTypes(false, false, false);

    // Wait for input (infinite loop)
    std::cout << "Reading MIDI input ... press <enter> to quit.\n";
    char input;
    std::cin.get(input);

    // Clean up
    delete midiin;

    return 0;
}
