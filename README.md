# Нативный компилятор
## Вступление
В этой работе я создам компилятор своего языка, который будет создавать исполняе
## Цели работы

## Frontend
В прошлом семестре я написал свой язык. Текстовый файл с синтаксисом моего языка представлялся в виде дерева, затем транслировался в ассемблер, который я написал
## Промежуточное представление (IR)
После фронтэнда программу стоит представить в виде промежуточного представления, чтобы было удобнее оптимизировать код и затем транслировать в бинарные команды.

Мое промежуточное имеет следующий вид:

1. Программа состоит из функций. Функция внутри себя содержит:

    * Массив переменных
    * Массив блоков комманд
    * Название функции


2. Каждый блок внутри себя содержит:
    * Массив команд
    * Название блока

3. Каждая команда состоит из:
    * Оператор 1
    * Оператор 2
    * Оператор 3

Здесь операторы это тоже структура, которая содержит в себе тип данных, значение для каждого типа. В операторе может лежать число, указатель на переменную в массиве переменных и указатель на блок (в случае переходов).

(Здесь нужен рисунок)
(Также можно привести тут картинку дампа)

## Трансляция в бинарный файл (Backend)
На этом этапе нужно массив команд в каждой функции транслировать в команды процессора. Для начала, запишем все в буфер, а далее создадим простой ELF файл и запишем туда. Ниже приведу примеры трансляции некоторый команд из моего промежуточного представления:

### Базовая арифметика
Строчка кода моего языка:

 ```z x tape y hp // z = x + y```

 Транслируется в промежуточное представление следующим образом:

```
    CMD     op1     op2     dest
    ADD     x       y       z
```
А в процессорные команды так:
```
    mov rax, [r9 - 8]  ; r9 pointer on buffer with vars. 8 - offset of x var
    mov rbx, [r9 - 16] ; 16 - offset of y var
    add rax, rbx
    mov [r9 - 24], rax
```
### Инициализация переменных
Строчка кода:
```
    var x 0 hp; // int x = 0
```
Представляется в промежутчном представлении:
```
    CMD     op1     op2    dest
    EQ       0       -      x
```
Транслируется в:
```
    mov [r9 - 8], 0
```
### Вызов функций
Вызов функции, представляющийся в промежуточном представлении:
```
    CMD     op1     op2     dest
    CALL    discriminant
```
### Условные переходы

## Тестирование производительности

## Вывод

