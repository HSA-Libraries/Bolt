/* COMPILE Command: "cl /EHsc StringifyKernels.cpp"
 * The generated exe is StringifyKernels.exe
 * Usage : StringifyKernels.exe <source-kernel-file> <destination-kernel-file> <kernel-string-name>
 */

#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{


	if(argc < 4)
	   std::cout << "USAGE: StringifyKernels.exe <source-kernel-file> <destination-kernel-file> <kernel-string-name>\n";

	const char *kernelFileName   = argv[1];
	const char *destFileName     = argv[2];
	const char *kernelStringName = argv[3];
	std::string line;
    //Open the Kernel file
	std::ifstream f_kernel(kernelFileName, (std::fstream::in));
	std::ofstream f_dest(destFileName, (std::fstream::out));
    if (f_kernel.is_open())
    {
		const std::string startLine = "const char *" + std::string(kernelStringName) + " = \\\n";
		f_dest.write(startLine.c_str(), startLine.length());
		while (!f_kernel.eof())
		{
			getline(f_kernel, line);
			const std::string destLine = "\"" + line + "\\n\"\n";
			const std::string tabSpaces = "    ";
			f_dest.write(tabSpaces.c_str(), tabSpaces.length());
			f_dest.write(destLine.c_str(), destLine.length());
		}
		const std::string endLine = "\"\";\n";
		f_dest.write(endLine.c_str(), endLine.length());
	}

    f_kernel.close();
}
