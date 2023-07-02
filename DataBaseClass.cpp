//#include "windows.h"

#include <iostream>
#include <format>

#include <pqxx/pqxx>


enum class ClientFields
{
	first_name = 1,
	last_name = 2,
	email = 3,
	phone_number = 4
};

class DataBaseClass
{
public:

	DataBaseClass(const std::string &host, const std::string &port, const std::string &dbname, const std::string &user, const std::string &password) : 
		connection ("host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password)
	{
		pqxx::work w(connection);
		//подготовка запросов, если БД уже существует
		if (w.query_value<bool>("SELECT EXISTS(SELECT * FROM pg_tables WHERE tablename = 'clients');") && 
			w.query_value<bool>("SELECT EXISTS(SELECT * FROM pg_tables WHERE tablename = 'emails');") && 
			w.query_value<bool>("SELECT EXISTS(SELECT * FROM pg_tables WHERE tablename = 'phone_numbers');"))
		{
			connection.prepare("client_insert", "INSERT INTO clients(id_client, first_name, last_name) VALUES($1, $2, $3);");
			connection.prepare("email_insert", "INSERT INTO emails(id_client, email) VALUES($1, $2);");
			connection.prepare("phone_insert", "INSERT INTO phone_numbers(id_client, phone_number) VALUES($1, $2);");

			connection.prepare("first_name_update", "UPDATE clients SET first_name = $2 WHERE id_client = $1;");
			connection.prepare("last_name_update", "UPDATE clients SET last_name = $2 WHERE id_client = $1;");
			connection.prepare("email_update", "UPDATE emails SET email = $2 WHERE id_client = $1;");
			connection.prepare("phone_update", "UPDATE phone_numbers SET phone_number = $2 WHERE id_client = $1;");

			connection.prepare("email_delete", "DELETE FROM emails WHERE email = $1;");
			connection.prepare("phone_delete", "DELETE FROM phone_numbers WHERE phone_number = $1;");
			connection.prepare("client_delete", "DELETE FROM clients WHERE id_client = $1;");
			connectionPrepared = true;
		}
	};

	bool create_tables() noexcept
	{
		try
		{
			pqxx::work w{ connection };
			w.exec(R"create(
				BEGIN;
				CREATE TABLE clients (
				id_client SERIAL PRIMARY KEY,
				first_name VARCHAR NOT NULL,
				last_name VARCHAR NOT NULL
				);

				CREATE TABLE emails(
				id_client INTEGER REFERENCES clients(id_client) ON UPDATE CASCADE ON DELETE CASCADE NOT NULL,
				email VARCHAR PRIMARY KEY
				);

				CREATE TABLE phone_numbers(
				id_client INTEGER REFERENCES clients(id_client) ON UPDATE CASCADE ON DELETE CASCADE NOT NULL,
				phone_number VARCHAR PRIMARY KEY
				);
				COMMIT;
			)create");
			
			if (!connectionPrepared)//подготовка запросов, если ранее не были созданы
			{
				connection.prepare("client_insert", "INSERT INTO clients(id_client, first_name, last_name) VALUES($1, $2, $3);");
				connection.prepare("email_insert", "INSERT INTO emails(id_client, email) VALUES($1, $2);");
				connection.prepare("phone_insert", "INSERT INTO phone_numbers(id_client, phone_number) VALUES($1, $2);");

				connection.prepare("first_name_update", "UPDATE clients SET first_name = $2 WHERE id_client = $1;");
				connection.prepare("last_name_update", "UPDATE clients SET last_name = $2 WHERE id_client = $1;");
				connection.prepare("email_update", "UPDATE emails SET email = $2 WHERE id_client = $1;");
				connection.prepare("phone_update", "UPDATE phone_numbers SET phone_number = $2 WHERE id_client = $1;");

				connection.prepare("email_delete", "DELETE FROM emails WHERE email = $1;");
				connection.prepare("phone_delete", "DELETE FROM phone_numbers WHERE phone_number = $1;");
				connection.prepare("client_delete", "DELETE FROM clients WHERE id_client = $1;");
			}			
			w.commit();
		}
		catch (const std::exception &err)
		{
			std::cout << "create_tables: Не удалось создать таблицы." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "create_tables: Не удалось создать таблицы. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;		
	};

	bool drop_tables() noexcept
	{
		try
		{
			pqxx::work w{ connection };
			w.exec(R"drop(
				DROP TABLE IF EXISTS emails;
				DROP TABLE IF EXISTS phone_numbers;
				DROP TABLE IF EXISTS clients;
			)drop");
			w.commit();
		}
		catch (const std::exception &err)
		{
			std::cout << "drop_tables: Не удалось удалить таблицы." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "drop_tables: Не удалось удалить таблицы. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;		
	};

	int add_client(const std::string &first_name, const std::string &last_name)//возвращает id = 0 при неудаче
	{
		int id_client = 0;
		try
		{
			pqxx::work w{ connection };
			id_client = w.query_value<int>(R"query(SELECT COUNT(id_client) FROM clients)query") + 1;
			w.exec_prepared("client_insert", id_client, first_name, last_name);
			w.commit();
		}
		catch (const std::exception &err)
		{
			std::cout << "add_client: Не удалось добавить клиента." << std::endl;
			std::cout << err.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "add_client: Не удалось добавить клиента. Неизвестная ошибка!" << std::endl;
		}
		return id_client;
	};

	bool add_phone_number(int id_client, const std::string &phone_number) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE id_client = " + std::to_string(id_client) + ";") > 0)//проверка существования id
			{
				w.exec_prepared("phone_insert", id_client, phone_number);
				w.commit();
			}
			else
			{
				std::cout << "add_phone_number: Указанный клиент не найден!" << std::endl;
				return 1;
			}
		}
		catch (const std::exception &err)
		{
			std::cout << "add_phone_number: Не удалось добавить номер телефона." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "add_phone_number: Не удалось добавить номер телефона. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};

	bool add_email(int id_client, const std::string &email) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE id_client = " + std::to_string(id_client) + ";") > 0)//проверка существования id
			{
				w.exec_prepared("email_insert", id_client, email);
				w.commit();
			}
			else
			{
				std::cout << "add_email: Указанный клиент не найден!" << std::endl;
				return 1;
			}
		}
		catch (const std::exception &err)
		{
			std::cout << "add_email: Не удалось добавить email." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "add_email: Не удалось добавить email. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};

	bool update_client(int id_client, ClientFields field, const std::string &value) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE id_client = " + std::to_string(id_client) + ";") > 0)//проверка существования id
			{
				switch (field)
				{
				case ClientFields::first_name:
					w.exec_prepared("first_name_update", id_client, value);
					break;
				case ClientFields::last_name:
					w.exec_prepared("last_name_update", id_client, value);
					break;
				case ClientFields::email:
					if (w.query_value<int>("SELECT COUNT(email) FROM emails WHERE id_client = " + std::to_string(id_client) + ";") == 0)//email не существует для указанного id
					{
						w.exec_prepared("email_insert", id_client, value);
					}
					else if (w.query_value<int>("SELECT COUNT(email) FROM emails WHERE id_client = " + std::to_string(id_client) + ";") == 1)//есть только один email для указанного id
					{
						w.exec_prepared("email_update", id_client, value);
					}
					else//есть несколько email для указаннорго id
					{
						std::cout << "У клиента более одного email, удалите неактуальные email и добавьте новый." << std::endl;
						return 1;
					}
					break;
				case ClientFields::phone_number:
					if (w.query_value<int>("SELECT COUNT(phone_number) FROM phone_numbers WHERE id_client = " + std::to_string(id_client) + ";") == 0)//телефон не существует для указанного id
					{
						w.exec_prepared("phone_insert", id_client, value);
					}
					else if (w.query_value<int>("SELECT COUNT(phone_number) FROM phone_numbers WHERE id_client = " + std::to_string(id_client) + ";") == 1)//есть только один телефон для указанного id
					{
						w.exec_prepared("phone_update", id_client, value);
					}
					else//есть несколько телефонов для указаннорго id
					{
						std::cout << "У клиента более одного номера телефона, удалите неактуальные номера и добавьте новый." << std::endl;
						return 1;
					}
					break;
				default:
					std::cout << "update_client: Некорректное значение поля field функции update_client." << std::endl;
					return 1;
					break;
				}
				w.commit();
			}
			else
			{
				std::cout << "update_client: Указанный клиент не найден!" << std::endl;
				return 1;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "update_client: Не удалось изменить данные клиента." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "Не удалось изменить данные клиента. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};

	bool delete_phone_number(const std::string &phone_number) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(phone_number) FROM phone_numbers WHERE phone_number = '" + w.esc(phone_number) + "';") > 0)//проверка существования телефона
			{
				w.exec_prepared("phone_delete", phone_number);
				w.commit();
			}
			else
			{
				std::cout << "delete_phone_number: Указанный телефон не найден!" << std::endl;
				return 1;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "delete_phone_number: Не удалось удалить номер телефона." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "delete_phone_number: Не удалось удалить номер телефона. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};

	bool delete_email(const std::string &email) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(email) FROM emails WHERE email = '" + w.esc(email) + "';") > 0)//проверка существования email
			{
				w.exec_prepared("email_delete", email);
				w.commit();
			}
			else
			{
				std::cout << "delete_email: Указанный email не найден!" << std::endl;
				return 1;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "delete_email: Не удалось удалить email." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "delete_email: Не удалось удалить email. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};

	bool delete_client(int id_client) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE id_client = " + std::to_string(id_client) + ";") > 0)//проверка существования id
			{
				w.exec_prepared("client_delete", id_client);
				w.commit();
			}
			else
			{
				std::cout << "delete_client: Указанный клиент не найден!" << std::endl;
				return 1;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "delete_client: Не удалось удалить клиента." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "delete_client: Не удалось удалить клиента. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;		
	};

	int find_client_name(const std::string &first_name, const std::string &last_name) noexcept //возвращает id = 0 при неудаче
	{
		int id_client = 0;
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE first_name = '" + w.esc(first_name) + 
				"' AND last_name = '" + w.esc(last_name) + "';") == 1)//существет один клиент с указанным Именем Фамилией
			{
				id_client = w.query_value<int>("SELECT c.id_client FROM clients c \
				WHERE first_name = '" + w.esc(first_name) + "' AND last_name = '" + w.esc(last_name) + "';");
			}
			else if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE first_name = '" + w.esc(first_name) + 
				"' AND last_name = '" + w.esc(last_name) + "';") > 1)//существет несколько клиентов с указанным Именем Фамилией
			{
				bool commited = 0;
				std::cout << "Найдено несколько клиентов c указанными именем и фамилией: " << std::endl;
				for (const auto &el : w.query<int>(" SELECT id_client \
					FROM clients \
					WHERE first_name = '" + w.esc(first_name) + "' AND last_name = '" + w.esc(last_name) + "';"))
				{					
					if (!commited)
					{
						w.commit();//вынужденное закрытие транзакции, чтобы выполнить печать print_client, где открывается новая транзакция
						commited = 1;
					}					
					print_client(std::get<0>(el));
					std::cout << std::endl;
					//pqxx::work w{ connection };
				}
			}
			else
			{
				std::cout << "find_client_name: Клиент с указанным именем и фамилией не найден!" << std::endl;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "find_client_name: Не удалось найти клиента." << std::endl;
			std::cout << err.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "find_client_name: Не удалось найти клиента. Неизвестная ошибка!" << std::endl;
		}
		return id_client;
	};

	int find_client_contact(const std::string &contact) noexcept //возвращает id = 0 при неудаче
	{
		int id_client = 0;
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM emails WHERE email = '" + contact + "';") > 0 ||
				w.query_value<int>("SELECT COUNT(id_client) FROM phone_numbers WHERE phone_number = '" + contact + "';") > 0)//проверка существования указанного email или телефона
			{
				id_client = w.query_value<int>("SELECT DISTINCT e.id_client FROM emails e \
				JOIN phone_numbers p ON p.id_client = e.id_client \
				WHERE email = '" + w.esc(contact) + "' OR phone_number = '" + w.esc(contact) + "';");
			}
			else
			{
				std::cout << "find_client_contact: Указанный контакт не найден!" << std::endl;
			}			
		}
		catch (const std::exception &err)
		{
			std::cout << "find_client_contact: Не удалось найти клиента." << std::endl;
			std::cout << err.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "find_client_contact: Не удалось найти клиента. Неизвестная ошибка!" << std::endl;
		}
		return id_client;
	};

	bool print_client(int id_client) noexcept
	{
		try
		{
			pqxx::work w{ connection };
			if (w.query_value<int>("SELECT COUNT(id_client) FROM clients WHERE id_client = " + std::to_string(id_client) + ";") > 0)//проверка существования id
			{
				std::cout << "id = " << id_client << std::endl;
				std::cout << "Имя: " << w.query_value<std::string>("SELECT first_name FROM clients WHERE id_client = " + std::to_string(id_client) + ";") << std::endl;
				std::cout << "Фамилия: " << w.query_value<std::string>("SELECT last_name FROM clients WHERE id_client = " + std::to_string(id_client) + ";") << std::endl;
				for (const auto &el : w.query<std::string>(R"query(
					SELECT email
					FROM emails
					WHERE id_client =)query" + std::to_string(id_client) + ";"))
				{
					std::cout << "Email: " << std::get<0>(el) << std::endl;
				}

				for (const auto &el : w.query<std::string>(R"query(
					SELECT phone_number
					FROM phone_numbers
					WHERE id_client =)query" + std::to_string(id_client) + ";"))
				{
					std::cout << "Телефон: " << std::get<0>(el) << std::endl;
				}

			}
			else
			{
				std::cout << "print_client: Указанный клиент не найден!" << std::endl;
				return 1;
			}

		}
		catch (const std::exception &err)
		{
			std::cout << "print_client: Не удалось отобразить данные клиента." << std::endl;
			std::cout << err.what() << std::endl;
			return 1;
		}
		catch (...)
		{
			std::cout << "print_client: Не удалось отобразить данные клиента. Неизвестная ошибка!" << std::endl;
			return 1;
		}
		return 0;
	};
private:
	pqxx::connection connection;
	bool connectionPrepared = 0;
};



int main(int argc, char** argv)
{
	//SetConsoleCP(65001);
	//SetConsoleOutputCP(65001);
	setlocale(LC_ALL, ".UTF8");
	int id;
	try
	{
		DataBaseClass my_db("localhost", "5432", "clients", "postgres", "123zxcZXC");
		my_db.drop_tables();
		my_db.create_tables();

		id = my_db.add_client("Иван", "Иванов");
		my_db.add_email(id, "ivanov1@mail.ru");
		my_db.add_email(id, "ivanov2@mail.ru");
		my_db.add_email(id, "ivanov3@mail.ru");
		my_db.add_phone_number(id, "+7111");
		my_db.add_phone_number(id, "+7112");
		my_db.add_phone_number(id, "+7113");

		id = my_db.add_client("Петр", "Петров");
		my_db.add_email(id, "petrov1@mail.ru");
		my_db.add_email(id, "petrov2@mail.ru");
		my_db.add_email(id, "petrov3@mail.ru");
		my_db.add_phone_number(id, "+7221");
		my_db.add_phone_number(id, "+7222");
		my_db.add_phone_number(id, "+7223");

		id = my_db.add_client("Вася", "Васильев");
		my_db.add_email(id, "vasilev1@mail.ru");
		my_db.add_email(id, "vasilev2@mail.ru");
		my_db.add_email(id, "vasilev3@mail.ru");
		my_db.add_phone_number(id, "+7331");
		my_db.add_phone_number(id, "+7332");
		my_db.add_phone_number(id, "+7333");

		id = my_db.add_client("Иван", "Иванов");
		my_db.add_email(id, "ivanov4@mail.ru");
		my_db.add_email(id, "ivanov5@mail.ru");
		my_db.add_email(id, "ivanov6@mail.ru");
		my_db.add_phone_number(id, "+7441");
		my_db.add_phone_number(id, "+7442");
		my_db.add_phone_number(id, "+7443");

		id = my_db.add_client("Дмтрий", "Фдоров");
		my_db.add_email(id, "fedorov@mail.ru");
		my_db.add_phone_number(id, "+7551");
		

		std::cout << "\x1B[36m" << "\n\nВывод всех созданных клиентов \n\n" << "\x1B[0m";
		my_db.print_client(1);
		std::cout << std::endl;
		my_db.print_client(2);
		std::cout << std::endl;
		my_db.print_client(3);
		std::cout << std::endl;
		my_db.print_client(4);
		std::cout << std::endl;
		my_db.print_client(5);
		std::cout << std::endl;

		std::cout << "\x1B[36m" << "\n\nПопытка удаления несуществующего email \n\n" << "\x1B[0m";
		my_db.delete_email("ivanov@mail5.ru");//несуществующий
		std::cout << "\x1B[36m" << "\n\nУдаляем ivanov1@mail.ru\n\n" << "\x1B[0m";
		my_db.delete_email("ivanov1@mail.ru");
		my_db.print_client(1);

		std::cout << "\x1B[36m" << "\n\nПопытка удаления несуществующего номер телефона \n\n" << "\x1B[0m";
		my_db.delete_phone_number("+7000");//несуществующий
		std::cout << "\x1B[36m" << "\n\nУдаляем +7111\n\n" << "\x1B[0m";
		my_db.delete_phone_number("+7111");
		my_db.print_client(1);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по имени/фамилии, неуспешно \n\n" << "\x1B[0m";
		id = my_db.find_client_name("аааа", "бббб");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по имени/фамилии, но таких клиента двое \n\n" << "\x1B[0m";
		id = my_db.find_client_name("Иван", "Иванов");
		my_db.print_client(id);
		
		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по имени/фамилии успешно \n\n" << "\x1B[0m";
		id = my_db.find_client_name("Петр", "Петров");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по email неуспешно \n\n" << "\x1B[0m";
		id = my_db.find_client_contact("@mail");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по email успешно \n\n" << "\x1B[0m";
		id = my_db.find_client_contact("ivanov4@mail.ru");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по телефону неуспешно \n\n" << "\x1B[0m";
		id = my_db.find_client_contact("@mail");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nПопытка найти клиента по телефону успешно \n\n" << "\x1B[0m";
		id = my_db.find_client_contact("ivanov4@mail.ru");
		my_db.print_client(id);

		std::cout << "\x1B[36m" << "\n\nУдаляем клиента id = 1 и пытаемся его вывести после удаления \n\n" << "\x1B[0m";
		my_db.delete_client(1);
		my_db.print_client(1);

		std::cout << "\x1B[36m" << "\n\nВывод всех оставшихся клиентов \n\n" << "\x1B[0m";
		my_db.print_client(2);
		my_db.print_client(3);
		my_db.print_client(4);
		my_db.print_client(5);


		std::cout << "\x1B[36m" << "\n\nОбновление клиента Дмтрий Фдоров-->Дмитрий Фёдоров; new_email; new_phone\n\n" << "\x1B[0m";
		id = my_db.find_client_contact("fedorov@mail.ru");
		my_db.update_client(id, ClientFields::first_name, "Дмитрий");
		my_db.update_client(id, ClientFields::last_name, "Фёдоров");
		my_db.update_client(id, ClientFields::email, "new_email");
		my_db.update_client(id, ClientFields::phone_number, "new_phone");

		my_db.print_client(id);
	}
	catch(const std::exception &err)
	{
		std::cout << err.what() << std::endl;
	}
	return 0;
}