#include <iostream>
#include <fstream>
#include "curl_easy.h"
#include <string>
#include <jsonv/value.hpp>
#include <jsonv/parse.hpp>
#include <jsonv/serialization.hpp>
#include <chrono>
#include <iomanip>
#include <ctime>

std::fstream& GetExchangeRates(std::fstream&);
bool CheckDataAge(std::ostream&, std::istream&, std::fstream&);
double GetRateFromFile(const jsonv::value&, const std::string&);
void GetCurrencyCode(std::istream&, std::ostream&, std::string&, const int&);
void PrintWelcome(std::ostream&);
const int AskForChoice(std::ostream&, std::istream&);
bool CheckDataAge(std::ostream&, std::istream&, std::fstream&);
void AbortOperation(std::ostream&output) { output << "Cancelling..." << std::endl; }
bool AskForConfirmation(std::istream&, std::ostream&);
void ErrorHandling(std::ostream&, const int&);
double PrintConversion(const double&, const double&, const int&);
double GetAmount(std::istream&, std::ostream&);
void PrintWelcome(std::ostream&);
void ParseFile(jsonv::value&, std::fstream&);

int main()
{   
    PrintWelcome(std::cout); bool stop = false, update;
    std::fstream exchange_rates;
    update = CheckDataAge(std::cout, std::cin, exchange_rates);
    if (update)
	GetExchangeRates(exchange_rates);
    jsonv::value data_file;
    ParseFile(data_file, exchange_rates);
    while(!stop){
	int choice; choice = AskForChoice(std::cout, std::cin);
	if (choice == 3)
	    stop = AskForConfirmation(std::cin, std::cout);
	else {
	    double amount_to_convert = GetAmount(std::cin, std::cout);
	    if (!amount_to_convert == 0.0){
		std::string currency_code;
		GetCurrencyCode(std::cin, std::cout, currency_code, choice - 1);
		if (!currency_code.empty()){
		    double rate = GetRateFromFile(data_file, currency_code);
		    if (rate != 0.0){
			std::cout << amount_to_convert << " ";
			std::cout << ((choice - 1) ? "USD" : currency_code);
			std::cout << " is equivalent to "<<
			PrintConversion(amount_to_convert, rate, choice - 1) << ((choice - 1) ?	currency_code : "USD");
			std::cout << std::endl;
		    }
		} else
		    AbortOperation(std::cout);
	    } else {
		AbortOperation(std::cout);
	    }
	} 
	
    }
    std::cout << "Have a great life" << std::endl;
    return 0;
}

std::fstream &GetExchangeRates(std::fstream &exchange_rates)
{
    // open the data file as an output stream to hold the exchange rate info
    exchange_rates.open("exchange_rates.json", std::fstream::out);
    //Create a writer to handle the stream
    curl::curl_writer writer(exchange_rates);
    // Pass it to the easy constructor and get the exchange rates
    curl::curl_easy easy(writer);
    
    easy.add(curl::curl_pair<CURLoption,string>(CURLOPT_URL, "https://openexchangerates.org/api/latest.json?app_id=f83a61578a414c8dacdaac3780b7ba6a"));
    easy.add(curl::curl_pair<CURLoption,long>(CURLOPT_FOLLOWLOCATION,1L));
    try {
	easy.perform();
    } catch (curl_easy_exception error) {
	error.print_traceback();
    }
    exchange_rates.close();
    return exchange_rates;
}
double GetRateFromFile(const jsonv::value &data_file, const std::string &currency_code)
{
    double rate;
    try{
	rate = jsonv::extract<double>(data_file.at("rates").at(currency_code));
    } catch (std::out_of_range err){
	ErrorHandling(std::cout, 3);
	rate = 0.0;
    }
    return rate;
}
void PrintWelcome(std::ostream &output)
{output << "Welcome! This converts to and from USD to other currencies!" << std::endl;return;}
void GetCurrencyCode(std::istream &input, std::ostream &output, std::string &currency_code, const int &mode)
{
    output << "What currency are you converting " << ((mode? "from":"to"))  << "?" << std::endl;
    output << "Please enter the three character currency code: "; input >> currency_code;
    while (!currency_code.empty() && currency_code.length() != 3){
	ErrorHandling(output, 2);
	input >> currency_code;
    }
    return;
}
bool CheckDataAge(std::ostream &output, std::istream &input, std::fstream &data_file)
{
    // open the file that holds the last download date as an input file
    data_file.open("data_file.txt", std::fstream::in);
    // read in the date
    std::string last_update;
    std::getline(data_file, last_update);
    // print an output message to the user, informing them of the last update
    // and asks if they'd like to update the exchange rate file
    output << "The data file was last updated on: " << last_update << std::endl;
    output << "Would you like to update the data file? (y/n)  ";
    //get user input, should be 'y' or 'n'
    char choice; input >> choice;
    //throw them in a loop to ensure that they will enter 'y' or 'n'
    while (choice != 'y' && choice != 'n'){
	output << "Please enter \'y\' or \'n\'; ";
	input >> choice;
    }
    // close the data, it does not need to be open as an ifstream anymore
    data_file.close();
    // update if the user entered 'y'
    if (choice == 'y'){
	// open the date file as an ofstream 
	data_file.open("data_file.txt", std::fstream::out);
	// get the current date and output it to the date file
	auto current_time = std::chrono::system_clock::now();
	std::time_t now = std::chrono::system_clock::to_time_t(current_time);
	data_file << std::put_time(std::localtime(&now), "%D");
	// close the date file
	data_file.close();
	// return true to update the exchange rates 
	return true;
    }
    // return false to not update the exchange rates
    else
	return false;
}
const int AskForChoice(std::ostream &output, std::istream &input){
    bool have_valid_response = false;
    
    output << "Please enter a number based on the operation you wish to perform:" << std::endl;
    output << "1 - Convert to USD" << std::endl;
    output << "2 - Convert from USD" << std::endl;
    output << "3 - Exit the program" << std::endl;
    output << "--------------------------------------" << std::endl;
    std::string response; int n;
    input >> response;
    while (!have_valid_response){
	try{
	    n = std::stoi(response);
	    if (n == 1 || n == 2 || n == 3)
		have_valid_response = true;
	    else
		ErrorHandling(output, 1);		
	} catch (std::invalid_argument) {
	    ErrorHandling(output, 1);
	}
	if (!have_valid_response)
	    input >> response;
    }
    return n;
}
void ErrorHandling(std::ostream &output, const int &error_number){
    switch(error_number){
    case 1:
	output << "Invalid response. Please enter \"1\" or \"2\" for your response: ";
	break;
    case 2:
	output << "Currency codes must be 3 characters. Please reenter it: " << std::endl;
	break;
    case 3:
	output << "You entered an invalid currency code. Please double check that the currency code is correctly entered." << std::endl;
	break;
    case 4:
	output << "Invalid response. Please enter \'y\' or \'n\' for your response: ";
	break;
    case 5:
	output << "Please enter a numeric amount to convert, or enter \'0\' to cancel the action: ";
	break;
    case 6:
	output << "You entered a number that was too big. Please try reentering it: ";
	break;
    default:
	output << "Some shit went south, we don't know what. We'll get back to you on that.";
	break;
    }
}
bool AskForConfirmation(std::istream &input, std::ostream &output){
    // ask if user really wants to exit the program
    output << "Are you sure you wish to exit the program? (y/n): ";
    char response; input >> response;
    // keep asking for a valid response until one is obtained
    while (response != 'n' && response != 'y'){
	// print an error message and ask for their response again
	ErrorHandling(output, 4);
	input >> response;
    }
    // return true or false based on their decision to determine
    // program flow
    if (response == 'y')
	return true;
    else
	return false;
}

double GetAmount(std::istream &input, std::ostream &output){
    double amount_to_convert; std::string input_buffer;
    output << "Please enter the amount to convert (if you wish to cancel this action, enter '0' without the apostrophes): ";
    input >> input_buffer; bool have_valid_input = false;
    while (!have_valid_input){	
	try {
	    amount_to_convert = std::stod(input_buffer);
	    have_valid_input = true;
	} catch (std::invalid_argument err) {
	    ErrorHandling(output, 5);
	} catch (std::out_of_range err) {
	    ErrorHandling(output, 6);
	}
	if (!have_valid_input)
	    input >> input_buffer;
    }
    return amount_to_convert;
}
double PrintConversion(const double &amount, const double &rate, const int &conversion_case)
{
    return conversion_case ? (amount / rate) : (amount * rate);
}

void ParseFile(jsonv::value &data_file, std::fstream &file)
{
    file.open("exchange_rates.json", std::fstream::in);
    data_file = jsonv::parse(file);
    file.close();
    return;
}
