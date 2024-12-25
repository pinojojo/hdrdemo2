#ifndef ICAMERA_HPP
#define ICAMERA_HPP

#include <string>
#include <functional>
#include <map>

namespace lzx
{
    class ICamera
    {
    public:
        using StateChangedCallback = std::function<void(const std::string &state, const std::string &value)>;

        virtual ~ICamera() {}
        virtual std::string label() = 0;                                                                                      // ip or name
        virtual bool open() = 0;                                                                                              // open camera
        virtual bool close() = 0;                                                                                             // close camera
        virtual bool start() = 0;                                                                                             // start capture
        virtual bool stop() = 0;                                                                                              // stop capture
        virtual bool snap() = 0;                                                                                              // snap a frame
        virtual bool streaming() { return false; }                                                                            // check if streaming
        virtual bool set(const std::string &name, double value) { return false; }                                             // set double
        virtual bool set(const std::string &name, int value) { return false; }                                                // set int
        virtual bool set(const std::string &name, bool value) { return false; }                                               // set bool
        virtual bool set(const std::string &name, const std::string &value) { return false; }                                 // set string
        virtual bool get(const std::string &name, double &value) { return false; }                                            // get double
        virtual bool get(const std::string &name, int &value) { return false; }                                               // get int
        virtual bool get(const std::string &name, bool &value) { return false; }                                              // get bool
        virtual bool get(const std::string &name, std::string &value) { return false; }                                       // get string
        virtual bool getFrame(unsigned char *buffer, int &width, int &height, int &channels, int &bitDepth) { return false; } // get latest frame
        virtual void setStateChangedCallback(StateChangedCallback callback) { stateChangedCallback = callback; }

    protected:
        void notifyStateChanged(const std::string &state, const std::string &value)
        {
            if (stateChangedCallback)
            {
                stateChangedCallback(state, value);
            }
        }

        StateChangedCallback stateChangedCallback;
    };
}

#endif