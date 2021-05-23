#include <iostream>
#include <string.h>
#include <stdio.h>
#include <windows.h>


//definiowane SUB SOH EOT ACK NAK C
#define SUB 26
#define SOH 0x1
#define EOT 0x4
#define ACK 0x6
#define NAK 0x15
#define C 'C'

//deklaracje
HANDLE handle;
DCB dcb;
bool ifCRC;
char* COM;

using namespace std;

void choose_port(int portchoice) { //funkcja do wyboru portow w mainie dla uzytkownika
    if(portchoice == 1) COM = "COM1";
    else if (portchoice == 2) COM = "COM2";
    else if (portchoice == 3) COM = "COM3";
    else if (portchoice == 4) COM = "COM4";
}

void przygotujPorty(char *choice)
{
    handle = CreateFile(choice, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    GetCommState(handle, &dcb);
    dcb.BaudRate = 9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
}

void odbieranieCOM(char *bufptr, int dl)
{
    //deklaracje
    DWORD baits;
    DWORD m = 0;

    while (dl > m)
    {
        ReadFile(handle, bufptr + m, dl - m, &baits, NULL);//odczytuje damne
        m = m+baits;
        cout<<" ";
    }
}

void przesylanieCOM(char *bufptr, int dl)
{
    //deklaracje
    DWORD baits;
    cout<<" ";
    WriteFile(handle, bufptr, dl, &baits, NULL);//Zapisuje dane do określonego pliku
    cout<<" ";
}

short int calculateCRC(char *bufpliku, int zlicz)
{
    unsigned short tmp;
    unsigned short tmp2;
    char tmp3;
    unsigned short contr;
    contr = 0;
    while (--zlicz >= 0) //zmienjaszanie az bedzie >=0
    {
        tmp3 = 8;
        contr = contr ^ (int) *bufpliku++ << 8;
        do
        {
            if (contr & 0x8000)
            {
                contr = contr<<1 ^ 0x1021;
            }
            else contr = contr << 1;
        } while (--tmp3);
    }
    tmp = contr>>8;
    tmp2 = contr^58368;
    tmp2= tmp2<<8;
    tmp2= tmp2+tmp;
    return (tmp2);
}

void wysylanie()
{
    //deklaracja zmiennych
    cout<<" ";
    int tmp = 1;
    unsigned char dl;
    unsigned short sum;
    int roznica = 255;
    int rozmpliku=0;
    char fragment[128];
    char buff[3];
    FILE *plik_tmp = fopen("plik.txt", "rb"); //zapis binarny do pliku z wiadomoscia
    fseek(plik_tmp, 0, SEEK_END); //przesuniecie kursora wzgledem konca pliku
    rozmpliku = ftell(plik_tmp); // rozmiar pliku - ustalenie
    cout<<" ";
    przygotujPorty(COM); //wywołanie przygotujPorty()
    cout<<" ";
    odbieranieCOM(buff, 1);
    cout<<" ";
    if (buff[0] == C) ifCRC = true;
    else if (buff[0] == NAK) ifCRC = false; //zainicjowanie transmisji
    fseek(plik_tmp, 0, SEEK_SET);//przesuniecie kursora wzgledem poczatku pliku

    while (rozmpliku > ftell(plik_tmp)) //warunek dotyczacy rozmiaru pliku
    {
        dl = fread(fragment, 1, 128, plik_tmp); //odczytanuie 128 elementow
        sum = 0;

        for (int i=dl;i<128;i++) fragment[i] = SUB;//ascii 26

        //wyliczenie sumy kontrtolnej lub CRC w zależności od wybranego wariantu
        if (ifCRC) sum = calculateCRC(fragment, 128);
        else
        {
            for (int i=0;i<128;i++) sum += (unsigned char) fragment[i];
            sum %= 256;
        }
        //deklaracje
        roznica = 255-tmp;
        buff[2] = roznica;
        buff[1] = tmp;
        buff[0] = SOH;
        //wywolywanie funkcji
        przesylanieCOM(buff, 3);
        przesylanieCOM(fragment, 128);
        przesylanieCOM((char *) &sum, ifCRC ? 2 : 1); //jesli spelniony to 2, jesli nie to 1
        odbieranieCOM(buff, 1);

        if (buff[0] != ACK) fseek(plik_tmp, -128, SEEK_CUR);//przesuniecie kursora wzgledem aktualnej pozycji
        else tmp=tmp+1;
    }
    fclose(plik_tmp); //zamkniecie pliku
    do
    {
        buff[0] = EOT; // wysyłanie znaku EOT do momentu ACK
        przesylanieCOM(buff, 1);
        odbieranieCOM(buff, 1);
    }
    while (buff[0] != ACK); // ACK musi byc spełnione by petla sie zakonczyla
}

void odbieranie()
{
    //deklaracje zmiennych
    char buff[3];
    char fragment[128];
    unsigned char koniec = 0;
    unsigned short kontrolna = 0;
    unsigned short sum = 0;

    przygotujPorty(COM);
    buff[0] = ifCRC ? C : NAK;//inicjowanie transmisji NAK
    przesylanieCOM(buff, 1);
    odbieranieCOM(buff, 1);  //odbior danych
    FILE *file_tmp = fopen("plik.txt", "wb"); //write binarny na pliku
    cout<<" ";
    cout<<" ";
    cout<<" ";
    for(;;) // do nieskończoności
    {
        //przypisanie poczatkowych wartosci do zmiennych
        koniec = 127;
        kontrolna = 0;
        sum = 0;
        cout<<" ";
        odbieranieCOM(buff + 1, 2);
        odbieranieCOM(fragment, 128);
        cout<<" ";
        cout<<" ";
        odbieranieCOM((char *) &sum, ifCRC ? 2 : 1);//jesli spelniony to 2, jesli nie to 1
        cout<<" ";

        if (!ifCRC) // suma kontrolna lub CRC
        {
            for (int i=0;i<128; i++) kontrolna = kontrolna + (unsigned char) fragment[i];
            kontrolna = kontrolna % 256; //wyliczenie sumu kontrolnej
        }
        else kontrolna = calculateCRC(fragment, 128); //ewentualnie CRC wedlug wyboru opcji w interfejsie

        if (sum == kontrolna) // jesli sumy kontrolne sa te same to kontunuuujemy
        {
            buff[0]=ACK; //potwierdzenie znakiem ACK jesli sume kontrolne bytly zgodne
            cout << " ";
            przesylanieCOM(buff, 1);
            cout << " ";
            odbieranieCOM(buff, 1);
            if (buff[0] == EOT) // EOT - koniec transmiji
            {
                while (fragment[koniec] == SUB) koniec = koniec - 1;
                fwrite(fragment, koniec + 1, 1, file_tmp);//Zwraca liczbę poprawnie zapisanych elementów
                break;
            }
            fwrite(fragment, 128, 1, file_tmp);//Zwraca liczbę poprawnie zapisanych elementów
        }
        else //jesli sumy nie sa rowne to wracamy wyżej
        {
            buff[0]=NAK;//przeslanie znaku NAK i powrot do blednie przeslanego bloku
            cout<<" ";
            przesylanieCOM(buff, 1);
            continue;
        }
    }
    cout<<" ";
    fclose(file_tmp); //zamkniecie pliku
    buff[0] = ACK; // potwierdzenie poprawnosci
    cout<<" ";
    przesylanieCOM(buff, 1);
}

int main()
{
    cout<<"Autorzy: Maciej Kolibabski i Kacper Swiader"<<endl;
    cout<<"Wybierz tryb: "<<endl;
    cout<<"1 - wysylanie"<<endl;
    cout<<"2 - odbieranie"<<endl;
    cout<<"Wybor: ";
    int choice;
    cin>>choice;
    cout<<"Uzywac CRC "<<endl;
    cout<<"T - tak"<<endl;
    cout<<"N - nie"<<endl;
    char crcchoice;
    cin>>crcchoice;
    if (crcchoice == 'T') ifCRC = true;
    else if (crcchoice == 'N') ifCRC = false;
    cout<<"Wybierz numer COM: "<<endl;
    int portchoice;
    cin>>portchoice;
    choose_port(portchoice);
//    if(portchoice == 1) COM = "COM1";
//    else if (portchoice == 2) COM = "COM2";
//    else if (portchoice == 3) COM = "COM3";
//    else if (portchoice == 4) COM = "COM4";
    if(choice == 1) wysylanie();
    else if (choice == 2) odbieranie();
    return 0;
}