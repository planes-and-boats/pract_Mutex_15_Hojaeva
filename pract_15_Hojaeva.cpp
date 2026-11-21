#include <iostream>
#include <Windows.h>
#include <fstream>
#include <exception>
#include <climits>
#include <string>

using namespace std;

#define MUTEX_MODE
#define FILE_NAME_BIN "account.bin"
#define FILE_NAME_LOG "operations.log"

struct BankAccount {
	int accountNumber;
	int balance;
};

enum Status {
	Completed,
	Refused
};

enum Type {
	Income,
	Outcome
};

void CreateFile() {
	ifstream chek(FILE_NAME_BIN, ios::binary);

	if (chek.is_open()) {
		chek.close();
		return;
	}
	chek.close();

	BankAccount account{ 10001, 10000 };
	ofstream file(FILE_NAME_BIN, ios::binary);

	if (!file.is_open()) {
		throw runtime_error("Не удалось создать файл счета.");
		return;
	}

	file.write(reinterpret_cast<const char*>(&account), sizeof(BankAccount));
	file.close();
}

void WriteFile(BankAccount& account) {
	ofstream file(FILE_NAME_BIN, ios::binary);

	if (!file.is_open()) {
		throw runtime_error("Не удалось открыть файл для записи счета.");
		return;
	}

	file.write(reinterpret_cast<const char*>(&account), sizeof(BankAccount));
	file.close();
}

void ReadFile(BankAccount& account) {
	ifstream file(FILE_NAME_BIN, ios::binary);

	if (!file.is_open()) {
		throw runtime_error("Не удалось открыть файл для чтения счета.");
	}

	file.read(reinterpret_cast<char*>(&account), sizeof(account));
	file.close();
}

void WriteLog(int startBalance, Type type,
	int amount, int endBalance, Status status) {
	ofstream file(FILE_NAME_LOG, ios::app);

	if (!file.is_open()) {
		throw runtime_error("Не удалось открыть журнал операций.");
		return;
	}

	file << GetCurrentProcessId() << "\t| " << startBalance << "\t| ";
	file << ((type == Type::Income) ? "Пополнение\t" : "Снятие\t") << "| ";
	file << amount << "\t| " << endBalance << "\t| ";
	file << ((status == Status::Completed) ? "Завершено" : "Отказано") << "\n";
	file.close();

	cout << "\t" << GetCurrentProcessId() << "\t| "
		<< startBalance << "\t| "
		<< (type == Type::Income ? "Пополнение\t| " : "Снятие\t| ")
		<< amount << "\t| "
		<< endBalance << "\t| "
		<< (status == Status::Completed ? "Завершено" : "Отказано") << endl;

}

void UpBalance(BankAccount& account, int amount) {
	Status status;
	int startBalance = account.balance;

	if (amount <= 0 || account.balance > INT_MAX - amount) {
		status = Status::Refused;
	}
	else {
		status = Status::Completed;
		account.balance += amount;
	}

	WriteLog(startBalance, Type::Income, amount, account.balance, status);
}

void DownBalance(BankAccount& account, int amount) {
	if (amount <= 0)
		return;
	int startBalance = account.balance;
	Status status;
	if (account.balance >= amount) {
		account.balance -= amount;
		status = Status::Completed;
	}
	else {
		status = Status::Refused;
	}

	WriteLog(startBalance, Type::Outcome, amount, account.balance, status);

}

void CatchMutex(HANDLE hMutex) {
	DWORD waitResult = WaitForSingleObject(hMutex, 3000);

	if (waitResult == WAIT_TIMEOUT) {
		throw runtime_error("Время ожидания операции истекло (3 сек).");
	}

	if (waitResult == WAIT_ABANDONED) {
		cout << " [ ВНИМАНИЕ ] Предыдущий сеанс завершен аварийно. Проверка счета..." << endl;
	}
}

void ThrowMutex(HANDLE hMutex) {
	ReleaseMutex(hMutex);
}


void ShowAccount(HANDLE hMutex, BankAccount& account) {
#ifdef MUTEX_MODE

	CatchMutex(hMutex);

#else

	Sleep(200);

#endif // MUTEX_MODE

	ReadFile(account);

	cout << " [ ВЫПИСКА ПО СЧЕТУ ]" << endl << endl;
	cout << "    Номер счета : " << account.accountNumber << endl;
	cout << "    Баланс      : " << account.balance << " руб." << endl;


#ifdef MUTEX_MODE

	ThrowMutex(hMutex);

#endif // MUTEX_MODE
}

void Transaction(HANDLE hMutex, BankAccount& account, Type type, int amount) {
#ifdef MUTEX_MODE

	CatchMutex(hMutex);

#else

	Sleep(200);

#endif // MUTEX_MODE

	ReadFile(account);

	if (account.accountNumber != 10001 || account.balance < 0)
		throw invalid_argument("Данные счета повреждены.");

	if (type == Type::Income) {
		UpBalance(account, amount);
	}
	else {
		DownBalance(account, amount);
	}

	WriteFile(account);

#ifdef MUTEX_MODE

	ThrowMutex(hMutex);

#endif // MUTEX_MODE
}

void RandomTransactions(HANDLE hMutex, BankAccount& account) {
	cout << " [ ИНФО ] Запущена серия из 20 операций..." << endl << endl;

	for (int i = 0; i < 20; i++) {
		Type type = rand() % 2 == 0 ? Type::Income : Type::Outcome;
		int amount = rand() % 901 + 100;

		Transaction(hMutex, account, type, amount);

		Sleep(rand() % 401 + 100);
	}

	cout << endl << " [ ИНФО ] Серия операций завершена." << endl;
}

void GetNumber(int& number) {
	string input;
	cout << " Введите сумму > ";
	cin >> input;
	cout << endl;
	try {
		number = stoi(input);
	}
	catch (exception) {
		throw invalid_argument("Ожидалось числовое значение.");
	}

	if (number <= 0) {
		throw invalid_argument("Сумма должна быть больше нуля.");
	}
}

int main()
{
	setlocale(NULL, "Russian");
	srand(time(0) + GetCurrentProcessId());
	try {

		cout << endl << " [ БАНКОВСКИЙ СЧЕТ · ТЕРМИНАЛ #" << GetCurrentProcessId() << " ]" << endl << endl;

		HANDLE hMutex = NULL;

#ifdef MUTEX_MODE

		hMutex = CreateMutex(NULL, FALSE, L"Local\\HojaevaMutex");
		if (hMutex == NULL) {
			throw runtime_error("Не удалось создать мютекс (код " + to_string(GetLastError()) + ").");
		}

#else

		Sleep(200);

#endif // MUTEX_MODE

		BankAccount account;
		CreateFile();
		ReadFile(account);

		string input;
		int choice, amount;

		bool working = true;

		while (working) {
			try {
				cout << endl << " Операции:" << endl;
				cout << "  1 · Проверить баланс" << endl;
				cout << "  2 · Пополнить счет" << endl;
				cout << "  3 · Снять наличные" << endl;
				cout << "  4 · Авторежим (20 операций)" << endl;
				cout << "  0 · Завершить работу" << endl << endl;

				cout << " Выберите действие > ";
				cin >> input;
				try {
					choice = stoi(input);
				}
				catch (exception) {
					throw invalid_argument("Ожидалось числовое значение.");
				}

				cout << endl;
				switch (choice) {
				case 1: {

					ShowAccount(hMutex, account);

					break;
				} case 2: {

					cout << " [ ПОПОЛНЕНИЕ СЧЕТА ]" << endl << endl;

					GetNumber(amount);

					cout << " [ ИНФО ] Запущена операция пополнения..." << endl << endl;
					Transaction(hMutex, account, Type::Income, amount);
					cout << endl << " [ ИНФО ] Операция завершена." << endl << endl;

					break;
				} case 3: {

					cout << " [ СНЯТИЕ НАЛИЧНЫХ ]" << endl << endl;

					GetNumber(amount);

					cout << " [ ИНФО ] Запущена операция снятия..." << endl << endl;
					Transaction(hMutex, account, Type::Outcome, amount);
					cout << endl << " [ ИНФО ] Операция завершена." << endl << endl;

					break;
				} case 4: {

					RandomTransactions(hMutex, account);

					break;
				} case 0: {
					cout << " [ ЗАВЕРШЕНИЕ РАБОТЫ ]" << endl;
					working = false;
					break;
				} default: {
					throw invalid_argument("Неверный пункт меню.");
				}
				}
			}
			catch (exception& e) {
				cout << endl << " [ ОШИБКА ] " << e.what() << endl;

#ifdef MUTEX_MODE

				ThrowMutex(hMutex);

#endif
			}
		}


#ifdef MUTEX_MODE

		CloseHandle(hMutex);

#endif // MUTEX_MODE

	}
	catch (exception& e) {
		cout << endl << " [ ОШИБКА ] " << e.what() << endl;
	}

	return 0;
}
