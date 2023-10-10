//
//  sys_process.h
//  Stockfish Line Sharpness
//
//  Created by Camillo Schenone on 03/10/2023.
//

#ifndef sys_process_h
#define sys_process_h

#include <iostream>
#include <unistd.h>
#include <poll.h>   // poll
#include <fcntl.h>  // fcntl
#include <signal.h> // kill

#define MAX_TIMEOUT_MS 1000*60*5

// TODO, add the logic for windows (T_T).
namespace System
{
    class Process
    {
    public:
        Process(const std::string &command) : command_(command)
        {
            // create pipes: populate the two out and in arrays[2] with "piped" fds.
            if (pipe(out_pipe_) == -1) throw std::runtime_error("Failed to create output pipe");
            if (pipe(in_pipe_)  == -1) throw std::runtime_error("Failed to create input pipe");
        }
        ~Process() noexcept
        {
            kill_(child_pid_);
        }
        Process(const Process & other)              = default;
        Process(Process && other)                   = default;
        Process& operator=(const Process & other)   = default;
        Process& operator=(Process && other)        = default;
        
        std::string get_command() const { return command_; } 
        
        void kill_(pid_t pid)
        {
            /*
             fds will be cleaned up by the OS. And we are exiting, so we don't need to free slots
             in the process table.
             
             Sometimes, child processes exit or are killed, but the kernel will hold on to their exit
             code until some other process claims it with wait() or waitpid().
             
             Using WNOHANG will make wait()/waitpid() non blocking, and will return the child pid if it
             died or 0 if it didn't.
             */
            if (forked_)
            {  // We actually have at least started the child. We don't care if it died right after
               // If it crashed or errored out somehow, it will still hang around until another process
               // waits for it and grabs the leftovers.
                int status;
                pid_t r;
                do {
                    r = waitpid(pid, &status, WNOHANG);
                // repeat if the process was interrupted somehow.
                } while (r == -1 && errno == EINTR);
                
                // the child is still running, so kill it and wait for it to die.
                if (r == 0) {
                    kill(pid, SIGKILL);
                    wait(NULL);
                }
                // if r != 0 and -1 the child already exited, we don't care how for now.
            }
        }
        
        bool is_alive()
        {
            // If the child died, by calling waitpid on it, we'll clear any remain.
            // So we need to make sure that if we destruct the process object, when calling
            // kill we are not accidentally killing another process.
            int status;
            pid_t r;
            do {
                r = waitpid(child_pid_, &status, WNOHANG);
                // repeat if the process was interrupted somehow.
            } while (r == -1 && errno == EINTR);

            if (r == 0) return true; // child is still alive.
            else if ( (r == -1 && errno == ECHILD ) || (r == child_pid_) ) 
            {
                // there is no child process to wait, or it crashed/exited/terminated otherwise.
                // all it means is that we no longer have a forked process to talk to.
                // so reset to the starting state.
                // (no need to close the fd, since they will be used again on another eventual fork)
                forked_ = false;
                child_pid_ = 0;
                return false;
            } else {
                throw std::runtime_error("Error: waitpid() failed");
            }
        }
        
        pid_t start(const char * const argv[])
        {
            
            /*
             * fork creates a new process, the process is an exact copy of the parent process, except:
             * -child has unique PID, different from the parent
             * -child has copy of parent descriptors, which reference the same objects
             * when sucessfully completed fork() returns a value of 0 to the child and the pid to the parent
             */
            pid_t process_p = fork();
            /*
             *   |------- p_parent space -----------|        |-------------- p_child space ---------|
             *   |       any  -> | out[1] (write)   |   ->   |    out[0] (read)  | (dup2) -> STDIN  |
             *   |       any  <- | in[0] (read)     |   <-   |    in[1] (write)  | (dup2) <- STDOUT |
             *                                        pipe's
             *                   | out[0] (unused)    space       out[1] (unused)|
             *                   | in[1] (unused)                 in[0] (unused) |
             *                   |_______________________________________________|
             * Each process has it's own copy of file descriptors which point to the same underlaying
             * object! But the parent and the child only use some of those file descriptors. So:
             * -In the parent, close out[0] and in[1] (the child space's fds)
             * -In the child, close out[1] and in[0] (the parent space's fd)
             */
            
            if (process_p)
            { // parent process
                std::cout << "Starting process with PID: " << process_p << '\n';
                
              // close the copy of fds used by the child, but held by the parent.
                close(out_pipe_[0]);
                close(in_pipe_[1]);
                
                child_pid_ = process_p;
                forked_ = true;
                
            }
            else
            { // child process
              // ignore the interrupt signals in the child process, the parent process will handle it.
                signal(SIGINT, SIG_IGN);
                
                // close the copy of the fds used by the parent, but held by the child
                if (close(out_pipe_[1]) == -1)
                    throw std::runtime_error("[child] failed to close outpipe");
                if (close(in_pipe_[0]) == -1)
                    throw std::runtime_error("[child] failed to close inpipe");
                
                // redirect the child's stdinput to the read end of the output pipe
                // the messages generated by the parent are passed on to the child as input.
                if (dup2(out_pipe_[0], STDIN_FILENO) == -1)
                    throw std::runtime_error("[child] failed to map STDIN to parent's outpipe");
                // then close the matched fd, since STDIN now points to the file.
                if (close(out_pipe_[0]) == -1)
                    throw std::runtime_error("[child] failed to close mapped outpipe");
                
                // redirect the child's stdoutput to the write end of the output pipe.
                // the messages generated by the child are mirrored to the parent's output.
                if (dup2(in_pipe_[1], STDOUT_FILENO) == -1)
                    throw std::runtime_error("[child] failed to map STDOUT to parent's inpipe");
                // then close the matched fd, since STDOUT now points to the file.
                if (close(in_pipe_[1]) == -1)
                    throw std::runtime_error("[child] failed to close mapped inpipe");
                
                /*
                 See https://developer.apple.com/forums/thread/738796
                 and https://developer.apple.com/forums/thread/737464?answerId=764686022#764686022
                 
                 exec*'ing proves to be quite a hairy ball in macOS. In this case
                 the debugger complaints since `ls` is a platform binary (what is that?)
                 and therefore can't be debugged.
                 ```
                 char *args[] = {"/bin/ls", "-r", "-t", "-l", (char *) 0 };
                 execv(args[0], args);
                 ```
                 !! This code still works correctly when started from a regular shell !!
                 !! It only has issue when running within the macOS developing framework !!
                 */
                
                /*
                 For now, hardcode the stockfish path for this machine, in the future
                 it should be passed as an argument, like so: $> ./line_sharpness $(which engine) position_fen
                 
                 Alternatively it could be obtained from within the program itself:
                 ```
                 const char * args[] = {"/bin/sh", "-c", "which stockfish", NULL};
                 execv(args[0], const_cast<char * const *>(args));
                 ```
                 */
                
                
                // the first argument is the path of the executable.
                if (execv(command_.c_str(), const_cast<char * const *>(argv)) == -1)
                {
                    std::string err_str {"[child] failed to start process with error code: "};
                    err_str.append(std::to_string(errno));
                    throw std::runtime_error(err_str);
                }
                
                // _exit(0); // explicitly not using exit(0).
            }
            
            return process_p;
        }
        
        // read the process output and put it back into a provided vector of strings.
        // If the expected string is specified, the function will scan the output
        // for the requested string, returning true if it finds it, or false if it times out.
        // if left empty, the function will return true on timeout.
        bool read(std::vector<std::string> &out_lines, 
                  const std::string &expected = "", int timeout_ms = 0)
        {
            out_lines.clear();
            std::string curr_line {};
            char buffer[1024];
            
            int poll_ret {};
            if (timeout_ms <= 0) timeout_ms = MAX_TIMEOUT_MS;
            // from the manual: POLLHUP is an output only flag, ignored in the .events bitmask
            pollfd fds { .fd = in_pipe_[0], .events = POLLIN };
            for(;;)
            {
                // check that we have something to read.
                do {
                    poll_ret = poll(&fds, 1, timeout_ms);
                } while (poll_ret == -1 && errno == EINTR);
                
                if (poll_ret == -1)
                {   std::cout << "errno: " << strerror(errno) << " ";
                    throw std::runtime_error("poll() failed: ");
                }
                // we timedout
                if (poll_ret == 0) {
                    if (!curr_line.empty())
                        out_lines.push_back(curr_line);
                    break;
                }

                /* 
                 When a pipe is closed from the other side (i.e. the child process terminates)
                 poll returns an revent of POLLIN/POLLHUP to signal "EOF".
                 highly OS specific: see http://www.greenend.org.uk/rjk/tech/poll.html.
                
                 Linux/SunOS: POLLHUP   MacOS/FreeBSD: POLLIN|POLLHUP      OpenBSD/etc: POLLIN
                 We check for both POLLIN and POLLHUP, in case there is still data to read.
                 We rely on read to tell us if we reach the EOF.
                */
                int bytes_read = 0;
                if ( (fds.revents & POLLIN) || (fds.events & POLLHUP))
                {
                    bytes_read = ::read(fds.fd, buffer, sizeof(buffer));
                    
                    // reached EOF (pipe was closed)
                    if (bytes_read == 0) {
                        if (!curr_line.empty())
                            out_lines.push_back(curr_line);
                        return false;
                    }
                    
                    for (int i = 0; i < bytes_read; i++) {
                        if (buffer[i] == '\n') {
                            out_lines.push_back(curr_line);
                            // check if the current line has the expected substring. If it does, stop here.
                            if (curr_line.empty()) continue;
                            if (!expected.empty()) {
                                if (curr_line.rfind(expected, 0) != std::string::npos)
                                    return true;
                            }
                                
                            curr_line.clear();
                        } else curr_line += buffer[i];
                    }
                }
            }
            // If we were looking for something specific we didn't find it.
            // otherwise we read everything there was and we timedout successfully.
            return expected.empty() ? true : false;
        }
        
        void send_command(std::string input)
        {
            if (!is_alive())
                throw std::runtime_error("Error: the process is not running");
            
            // ensure we are sending a '\n' character.
            if (input[input.size()] != '\n')
                input.append("\n");
            
//            std::cout << ">> " << input;
            if ( write(out_pipe_[1], input.c_str(), input.size()) == -1 )
                throw std::runtime_error("Error: could not send command to the process");
        }
        
        // the mirror logic needs to be reworked. when the mirror reads from the in pipe, it steals
        // the data from pipe for the parent process. we need to duplicate it somehow.
//        void stop_mirror()
//        {
//            kill_(mirror_pid_);
//        }
//        
//        void mirror_output()
//        {
//            
//            pid_t mirror_p = fork();
//            if (mirror_p) {
//                mirror_pid_ = mirror_p;
//            } else {
//                // make a fd non-blocking (only useful if we use a tight read loop, not when polling).
//                // fcntl(in_pipe_[0], F_SETFL, fcntl(in_pipe_[0], F_GETFL) | O_NONBLOCK);
//                
//                char buffer[2048];
//                std::string curr_line {};
//                
//                while (true)
//                {
//                    // we use pread instead of read, so that we can use the same data again in subsequent reads.
//                    const int bytes_read = ::read(in_pipe_[0], buffer, sizeof(buffer));
//                    
//                    if (bytes_read == -1 && errno == EAGAIN) continue;
//                    if (bytes_read == -1 && errno != EAGAIN)
//                    {
//                        std::cout << "errno: " << strerror(errno) << " ";
//                        throw std::runtime_error("read() failed");
//                    }
//                    if (bytes_read == 0)
//                    {
//                        std::cout << ">> " << curr_line << std::endl;
//                        break;
//                    }
//                    
//                    for (int i = 0; i < bytes_read; i++) {
//                        if (buffer[i] == '\n') {
//                            std::cout << ">> " << curr_line << std::endl;
//                            curr_line += '\n';
//                            write(in_pipe_[1], curr_line.c_str(), curr_line.size());
//                            curr_line.clear();
//                        } else {
//                            curr_line += buffer[i];
//                        }
//                    }
//                }
//            }
//        }
        
    public:
        std::string command_;
    private:
        int out_pipe_[2];
        int in_pipe_[2];
        bool forked_ = false;
        pid_t child_pid_ = 0;
    };
}



#endif /* sys_process_h */
