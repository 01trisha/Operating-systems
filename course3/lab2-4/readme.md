Сделайте “грубую” реализацию спинлока и мутекса при помощи cas-функции и futex. 
Используйте их для синхронизации доступа к разделяемому ресурсу. Объясните принцип их работы.

atomic_int - переменная для которой будут применяться атомарные операции, store, load, fetch add

атомарность - единое целое, неделимое, в нее не могут вмешаться во время исполнения

``` x86_64 clang
spinlock_init:
        push    rbp
        mov     rbp, rsp
        mov     qword ptr [rbp - 8], rdi
        mov     rcx, qword ptr [rbp - 8]
        mov     dword ptr [rbp - 12], 0
        mov     eax, dword ptr [rbp - 12]
        xchg    dword ptr [rcx], eax
        pop     rbp
        ret

spinlock_lock:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32
        mov     qword ptr [rbp - 8], rdi
.LBB1_1:
        mov     dword ptr [rbp - 12], 0
        mov     rcx, qword ptr [rbp - 8] //указатель на s->lock
        mov     dword ptr [rbp - 16], 1 //новое значение 1
        mov     eax, dword ptr [rbp - 12] //expected(0)
        mov     edx, dword ptr [rbp - 16] //новое значений (1)
        lock            cmpxchg dword ptr [rcx], edx //лочим память(memory barrier), делаем команду compare & exchange,
                                                      // сравниваем eax с rcx, если равны то записываем edx в rcx, если не равны то
                                                      // rcx в eax
        mov     ecx, eax
        sete    al
        mov     byte ptr [rbp - 25], al
        mov     dword ptr [rbp - 24], ecx
        test    al, 1
        jne     .LBB1_3
        mov     eax, dword ptr [rbp - 24]
        mov     dword ptr [rbp - 12], eax
.LBB1_3:
        mov     al, byte ptr [rbp - 25]
        and     al, 1
        mov     byte ptr [rbp - 17], al
        test    byte ptr [rbp - 17], 1
        je      .LBB1_5
        jmp     .LBB1_6
.LBB1_5:
        call    sched_yield@PLT
        jmp     .LBB1_1
.LBB1_6:
        add     rsp, 32
        pop     rbp
        ret

spinlock_unlock:
        push    rbp
        mov     rbp, rsp
        mov     qword ptr [rbp - 8], rdi
        mov     rcx, qword ptr [rbp - 8]
        mov     dword ptr [rbp - 12], 0
        mov     eax, dword ptr [rbp - 12]
        xchg    dword ptr [rcx], eax
        pop     rbp
        ret
```


про спинлок: 
инициализируем, 0 в лок(разблочен)

блок:
постоянно ждем пока lock будет 0 и пишем 1(блокируем)
если получилось то выходим, иначе в цикле продолжаем(отдаем выполнение)

анлок - 0 в лок

про мьютекс:

futex syscall:

     syscall(SYS_futex, futex, operation, val, timeout, uaddr2, val3)

     - futex - указатель на atomic переменную в памяти
     - operation - операция (FUTEX_WAIT или FUTEX_WAKE)
     - val - значение для сравнения/количество потоков
     - timeout - таймаут (NULL = без таймаута)
     - uaddr2 - дополнительный адрес (не используется)
     - val3 - дополнительное значение (не используется)


второй случай:
1 поток: mutex_lock() -> CAS(0->1) успешно -> futex=1
2 поток: mutex_lock() -> CAS(1->1) неудача -> expected=1
 while(1):
   expected==1 -> CAS(1->2) успешно -> futex=2
   futex_wait(2) → засыпает
1 поток: mutex_unlock() -> exchange(2->0) -> old=2
 old==2 -> futex_wake(1) -> будит B
2 поток: просыпается
 expected=0 -> CAS(0->2) успешно → захватил



1 поток: mutex_lock() -> захватил (futex=1)
2 поток: mutex_lock() -> CAS(1->2) -> futex_wait(2)
1 поток: mutex_unlock() -> exchange(2->0) -> futex_wake(1)

**FUTEX_WAIT**

Эта операция автоматически проверяет, что адрес футекса все еще содержит заданное значение,
и спит в ожидании _FUTEX_WAKE_ на этом адресе футекса. Если аргумент _timeout_ не равен NULL,
то его содержимое описывает максимальную задержку ожидания, иначе это время бесконечно.
По **[futex](https://www.opennet.ru/cgi-bin/opennet/man.cgi?topic=futex&category=4)**(4) этот вызов исполняется, 
если уменьшение счетчика дает отрицательное значение (указывая на наличие спора), или будет спать, 
пока другой процесс не освободит футекс и не исполнит операцию FUTEX_WAKE.

**FUTEX_WAKE**

Эта операция пробуждает по большей части _val_ процессов, 
ожидающих на этом адресе футекса (т.е. внутри _FUTEX_WAIT_).
По **[futex](https://www.opennet.ru/cgi-bin/opennet/man.cgi?topic=futex&category=4)**(4) этот вызов исполняется,
если увеличение счетчика показало, что есть ожидающие, как только значение футекса стало равным 1
(обозначая что есть доступные ожидающие).

Futex:
Футекс идентифицируется блоком памяти, который может разделяться между разными процессами.
В этих разных процессах он необязательно имеет одинаковые адреса. В еще более чистой форме,
футекс имеет семантику семафоров; это счетчик, автоматически уменьшающийся или увеличивающийся;
процессы могут ожидать изменения значения на положительное.

Операцией футекса является полное пространство пользователя для бесспорных случаев.
Ядро привлекается только в случае конфликтов.

В его чистом виде футекс является выровненным целым, изменяемым только атомными 
ассемблерными инструкциями. Процессы могут разделять это целое поверх mmap,
через разделяемые сегменты или потому что разделяется пространство памяти,
в этом случае приложение обычно называется многопоточным.  



