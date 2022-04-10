# Mochawk
Canberra Genie2000 S561 spectroscopy script for summing and analyzing spectra of gamma radioactivity


# Purpose of the program

The precision of gamma spectrum measurement is time-dependant. It means, for the same sample with the same radionuclide composition, a spectrum acquired in a shorter time span will have greater errors and higher MDAs for the same radionuclides, compared with a spectrum acquired in a longer time span. Therefore, it is advantageous to have a long measurement time – to be able to identify as much radionuclides as possible with the lowest errors. However, if the state is dynamic and composition of radionuclides might change rapidly over time, measurements with longer acquisition time will not present the changes in reliable way. Solution for this problem is using “floating” spectra – every spectrum is measured in shorter timespans and the spectra’s channel data are further summed into one larger spectrum. This way we can achieve a sufficient sampling frequency with proper reliable results. 

A source detector creates spectra in a cycle. The cycle last, for example, 8 hours and the spectrum is created, for example, every 10 minutes. The created spectrum contains data acquired from the start of the cycle until the creation of the spectrum. To obtain a data always from 100 minutes of acquisition, the spectra obtained from the source have to be compared, summed, and analysed again. This procedure is done by Mochawk.

# Structure

A whole batch of necessary files consists of the following files:
1. Mochawk.exe – the main script running Mochawk.
2. Archive – a folders containing archived unprocessed spectra.
3. Summed – a folder containing processed spectra.
4. New – a folder containing obtained and yet not processed spectra.
5. Buffer – a folder containing spectra being “in memory” of the program.
6. Export – a folder containing files to be sent via FTP.
7. Flush – a technical folder for safe “cleaning” the other folders.
8. config.csv – a file containing overall parameters for processing the data.
9. ftp.csv – a file containing FTP logins for FTP import and export of the data.
10.  signals.csv – a file containing list of signals to be sent to other systems.
11.  Backup.txt – a file containing names of spectra being “in memory” of the program.
12.  display.html – a file containing full results from the last Gamma Client analysis.
13.  strip.exe – Genie2000 script, taken to the local folder from C:/GENIE2K/EXEFILES/

Additionally, there have to be an .ASF file for analysis steps (a path to the .ASF is defined in “config.csv” file) and at least an .NLB file with nuclides library (this file have to be places in C:/GENIE2K/CAMFILES/).
