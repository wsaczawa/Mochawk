# Mochawk
Canberra Genie2000 S561 spectroscopy script for summing and analyzing spectra of gamma radioactivity

"Mochawk" created by Wiktor Saczawa for Mochovce NPP, Slovakia. Tested by 3 months running trial with real data from the primary circuit coolant, without error.

In case of question or problems, feel free to contact the author: wiktor.saczawa@gmail.com.

# Purpose of the program

The main purpose of the program is to summarize and analyze gamma radioactivity spectra. In NPPs and other nuclear plants there are particulate systems, where gamma spectra are collected and analyzed (for example activities of requested radioisotopes are determined). The most simple solution for an on-line monitoring of a radioactive system is to set a gamma detector, which will permanently collect data and create spectra in declared periods, for example every 10 minutes. Setting a proper cycle time is a compromise between inertia of the system, when the time is long, and high MDAs (minimum detectable activity), when the time is short. A solution for such a dilemma is setting the cycle time as short as requested by the most critical condition from system's response point of view, and then transfer the spectra to a "black box", which will summarize the spectra. By summarizing channel data from a few spectra we can obtain a new spectrum, which might by analyzed with significantly lower MDAs. This program presents a solution, which might work as the mentioned "black box".

# Overall idea of the program

There are two attitudes towards solving problem of summarizing gamma spectra: spectra can be summarized periodically or continually. In periodical attitude, the more simple one, system waits for collecting enough spectra to summarize them and when it receives enough data, it performs a summarizing and analysis, then it waits for another batch of data. For example, if our raw data consists of 10 min. spectra, and we want to summarize them to 100 min. spectra, a system must wait for 10 spectra to be obtained, and summarized data will be obtained every 100 min. 

"Mochawk" presents different approach. In continually attitude, the program summarizes every new spectrum with previously received spectra, kept in the program's memory. Once again, if we have raw 10 min. spectra and we want to summarize them into 100 min. spectra, the program, receiving a new spectrum, will summarize new spectrum with 9 previously received spectra. It means, that the summarized data will be obtained every 10 min., thus the problem of long waiting time for summarized data is solved.

The program is written for people, who are slightly familiar with programming, but need to easy apply and modified C++ script for their Genie2000 purposes. Therefore whole script is put into one C++ sheet and objective programming is mixed with functional. A lot of things might be coded more elegant, but the code is designed to work even with the oldest version of Genie2000 S561 batch in fast and efficient way without errors. 

# Description of the program

The program is designed to work in cycles. Initialization of the program is not included in the C++ script and should be solved by a system program, for example Windows Task Scheduler, for maximizing system reliability in case of hardware difficulties (blackout, human mistake, reset of the PC, etc.). 

To "Mochawk" are attached four folders named: Archive, New, Backup and Flush. "Archive" contains .CNF files (spectra) received in the past (before particulate cycle has been run). "New" contains .CNF files, where new spectra are imported via FTP protocol. "Backup" contains .CNF files of spectra, which are proceeded as previous spectra for summation. "Flush" is void, technical folder for "flushing" content of other folders. Additionaly, a .txt file called "Backup" is attached, where names of .CNF files are stored. A spectrum called "00000.CNF" works as a working .CNF file for putting the summarized data. 

In the first step, the backup is loaded. Based on names stored in backup.txt, the program copy relevant .CNF files from "Archive" to "Backup". If it's first cycle of work, backup.txt will be void (as well as "Archive" folder), so no spectra will be transferred. Backup folder is cleaned always at the beginning of new cycle; that's a safety function to prevent loss of data in case of unpredictable shutdown of the program. 

In the next step, data from every spectrum in "Backup" is transferred to a vector containing spectrum data; these data are: name of the spectrum, time of creation, live time, real time, volume of the sample, and number of counts for every channel from 1 to 4096. The vector with the data is then sorted according to time of creation of spectra.

The next step is import of new spectra from an external gateway via FTP. If in "New" folder there are already more than 5 spectra, this step will be skipped in this cycle (that's a safety function in case of importing to many data). The script copy .CNF files from the gateway to folder "New" and then delete the .CNF in the gateway. Maximally 20 spectra in one cycle can be processed. In case of failure of copying of deleting, the program will try to repeat an attempt with 10 second interval as many times, as declared in a variable "CopyThreshold" or "DeleteThreshold" (fourth and the last variables of the ImportFtp funtion). After importing all (max. 20) files into local "New" folder, all imported files will be copied to "Archive" folder. 

In the next step, data from every spectrum in "New" is transferred to a vector, similar to the backup vector. The same data as previously are transferred. The vector is also sorted by time of creation of the spectra.

The next step of the algorithm is the main loop of the program. For every spectrum in the new spectra vector, from the oldest to the newest, following procedure is performed: First, the time consistency with the last spectrum in the buffer is controlled. If the new spectrum is older than the last spectrum in the buffer, the new spectrum is ignored and the loop procedure starts for next spectrum in the new spectra vector. If the new spectrum is younger than the last spectrum in the buffer, the time difference between those two spectra is important: if the time distance is too long, it means, that obtained spectra and archived spectra are unrepresentative for summing (for example, the buffer might remeber old spectra before an outage and new spectra come already during new campain), therefore the buffer is cleared and the procedure continues. If the time distance between the new spectrum and the old spectrum is in the defined threshold, procedure just continues. 

The data from the spectra are added to the buffer. If the buffer is full (there is data from more spectra than demanded), the data from the oldest spectra in the buffer will be removed. This is important for understanding, what results we can await at the begging of the cycle: first summing results will be just results for one spectrum. Then for two spectra, then for three, etc. until the buffer will contain data for 10 spectra (or any other number defined in a variable "SpectraLimit"). After that, as long as transfer of new spectra will be stable, we can await obtaining summing data for ten spectra.

Then the channel data, real times and live times in the buffer are summed and put in the working .CNF file. Quantity is taken from the newest spectrum in the buffer (but should be equal for all of the spectra). Then the working .CNF file with summed data is analyzed according to a sequence defined in an .ASF file, which is coupled with nuclides library defined in a .NBL file. After analyzing, the nuclide data is stored in the working .CNF file. Function "GetNuclides" put data from the working .CNF file into the nuclides vector. The data consists of the nuclide name, bulk activity, error and MDA. 

When the analyzes is done and the data is collected, obtained data can be presented and sent to a requested gateway via FTP. At this point the buffer (buffer.txt file) is updated, so interrupting the program would not lead to a data loss. The script proposes showing the data on screen during working of the program for on-line tracking, as well as putting the data into a HTML file for off-line tracking. The timestamp for the data is the time of creation of the newest spectrum in the buffer. Additionally, a .txt file with chosen data can be created, which can be further sent via FTP.

At the end, "New" folder is cleared. If "Archive" folder contains more than 100 spectra, all spectra are removed except the last (the newest) 15. 

# How to use this script?

"Mochawk" works on Windows 7 with Canberra Genie2000 VDM and installed S561 batch from 2001 or older.

First, adapt the script according to your purposes, especially modify username and password for FTP communication, as well as paths to different files. I recommend for compiling Microsoft Visual Studio. Put .exe file into folder destined for Mochawk and remember to configure Windows Task Manager to run the program from proper path. Wrong configuration will lead to corrupted FTP transfer. FTP will not work also during trial run in compilation, because of .exe file placement. .ASF and .NBL files must be adapted to your purpose and detector details. Path and name of the .CNF working file might be also important, name "00000.CNF" and putting to Desktop works, but any other configuration leaded to problems with analyzes with the .ASF file. 
