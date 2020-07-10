# BufferStream Library
A simple single-header library containing useful utils used to write data to buffers just like in the Source Engine net library. 

Example usage: 
```cpp
IOBufferStream in; // Initialize a buffer stream with standard size - 512.
in.Write(0x12ABCDEF);
std::string input;
std::cin >> input;
in.WriteString(input);

IOBufferStream out(in.Data(), in.Size()); // Initialize a buffer stream from the input stream reated before
int header = out.Read<int>();
std::string str = out.ReadString();
printf("Header: %X;\nString: %s\n", header, string.c_str());
```

This library provides automatic input buffer resizing, this means, if your buffer is too small to write something - dont worry, it will be resized! This will affect performance though. But this is not critical, if you are not using the buffer rapidly.

This library is good for network communication using sockets. And will probably be used in one of my future projects related to streams.

NOTE: Please, do not use read functions in variadic argumented functions like printf. It somewhy makes the functions get called in a reverse order. The reason behind thid might be the `__cdecl` function declaration, as if i remember correctly, it reads the arguments in reverse order. 
