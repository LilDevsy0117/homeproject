#include <iostream>
#include <string>
#include <stdexcept>
#include <map>
#include <cassert>

using namespace std;

class IBankGateway {
public:
	virtual ~IBankGateway() = default;
	virtual bool validatePin(const string& cardNumber, const string& pin) = 0;
	virtual int  getBalance(const string& accountId) = 0;
	virtual void debit(const string& accountId, int amount) = 0;
	virtual void credit(const string& accountId, int amount) = 0;
};

class ICashDispenser {
public:
	virtual ~ICashDispenser() = default;
	virtual bool hasCash(int amount) = 0;
	virtual void dispenseCash(int amount) = 0;
};

class ICardReader {
public:
	virtual ~ICardReader() = default;
	virtual string readCard() = 0;
	virtual void   ejectCard() = 0;
};

class ATMController {
	IBankGateway&    bank;
	ICashDispenser&  dispenser;
	ICardReader&     reader;

	string  cardNumber;
	bool    authenticated = false;
	string  selectedAccount;

public:
	ATMController(IBankGateway& b, ICashDispenser& d, ICardReader& r)
		: bank(b), dispenser(d), reader(r) {}

	void insertCard() {
		cardNumber = reader.readCard();
		authenticated = false;
		selectedAccount.clear();
	}

	bool enterPin(const string& pin) {
		if (cardNumber.empty())
			throw runtime_error("No card inserted");
		authenticated = bank.validatePin(cardNumber, pin);
		return authenticated;
	}

	void selectAccount(const string& acctId) {
		if (!authenticated)
			throw runtime_error("Not authenticated");
		selectedAccount = acctId;
	}

	int getBalance() {
		if (selectedAccount.empty())
			throw runtime_error("No account selected");
		return bank.getBalance(selectedAccount);
	}

	void deposit(int amount) {
		if (selectedAccount.empty())
			throw runtime_error("No account selected");
		if (amount <= 0)
			throw runtime_error("Invalid deposit amount");
		bank.credit(selectedAccount, amount);
	}

	void withdraw(int amount) {
		if (selectedAccount.empty())
			throw runtime_error("No account selected");
		if (amount <= 0)
			throw runtime_error("Invalid withdraw amount");
		int bal = bank.getBalance(selectedAccount);
		if (amount > bal)
			throw runtime_error("Insufficient funds");
		if (!dispenser.hasCash(amount))
			throw runtime_error("ATM has insufficient cash");
		bank.debit(selectedAccount, amount);
		dispenser.dispenseCash(amount);
	}

	void ejectCard() {
		reader.ejectCard();
		cardNumber.clear();
		authenticated = false;
		selectedAccount.clear();
	}
};

class MockBankGateway : public IBankGateway {
	map<string, string>    pins;
	map<string, int>       balances;

public:
	MockBankGateway() {
		pins["CARD-1234"] = "4321";
		balances["ACC-111"] = 100;
	}

	bool validatePin(const string& cardNumber, const string& pin) override {
		auto it = pins.find(cardNumber);
		return it != pins.end() && it->second == pin;
	}

	int getBalance(const string& accountId) override {
		return balances.at(accountId);
	}

	void debit(const string& accountId, int amount) override {
		balances[accountId] -= amount;
	}

	void credit(const string& accountId, int amount) override {
		balances[accountId] += amount;
	}
};

class MockCashDispenser : public ICashDispenser {
	int cashOnHand = 200;

public:
	bool hasCash(int amount) override {
		return amount <= cashOnHand;
	}
	void dispenseCash(int amount) override {
		if (amount > cashOnHand) throw runtime_error("Not enough ATM cash");
		cashOnHand -= amount;
	}
};

class MockCardReader : public ICardReader {
public:
	string readCard() override {
		return "CARD-1234";
	}
	void ejectCard() override {
	}
};

int main() {
	MockBankGateway   bank;
	MockCashDispenser disp;
	MockCardReader    reader;

	ATMController atm(bank, disp, reader);

	atm.insertCard();
	assert(!atm.enterPin("0000"));
	assert(atm.enterPin("4321"));

	atm.selectAccount("ACC-111");
	assert(atm.getBalance() == 100);

	atm.deposit(50);
	assert(atm.getBalance() == 150);

	atm.withdraw(70);
	assert(atm.getBalance() == 80);

	bool threw = false;
	try { atm.withdraw(200); }
	catch (...) { threw = true; }
	assert(threw);

	atm.ejectCard();

	cout << "All ATMController tests passed!\n";
	return 0;
}
