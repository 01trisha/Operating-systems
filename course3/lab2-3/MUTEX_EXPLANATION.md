# Объяснение работы потоков в реализации с Mutex

В этой программе используются 6 потоков: 3 потока для чтения (ASC, DESC, EQ) и 3 потока для изменения списка (SWAP).

## Структура данных

- **Storage** содержит указатель на первый элемент списка и `head_mutex` для защиты головы списка
- **Node** - узел списка, каждый имеет свой `sync` (mutex) для защиты
- **global_mutex** - для синхронизации доступа к глобальным переменным (счетчики, результаты)

---

## 1. Поток ASC (ascending_thread) - поиск возрастающих пар

**Задача:** Подсчитать пары соседних узлов, где длина строки первого узла **меньше** второго.

### Этапы работы:

1. **Захват головы списка:**
   ```
   pthread_mutex_lock(&storage->head_mutex)  // блокируем head_mutex
   curr = storage->first                      // получаем первый узел
   ```

2. **Проверка на пустоту:**
   ```
   if (curr == NULL)
       pthread_mutex_unlock(&storage->head_mutex)
       continue  // список пуст, повторяем попытку
   ```

3. **Hand-over-hand блокировка (техника "рука за руку"):**
   ```
   pthread_mutex_lock(&curr->sync)            // блокируем первый узел
   pthread_mutex_unlock(&storage->head_mutex) // освобождаем head_mutex
   ```
   *Теперь head_mutex свободен, другие потоки могут читать голову списка*

4. **Проход по списку:**
   ```
   next = curr->next
   while (next != NULL):
       pthread_mutex_lock(&next->sync)        // блокируем следующий узел
       
       if (strlen(curr->value) < strlen(next->value))
           local_count++                       // нашли возрастающую пару
       
       pthread_mutex_unlock(&curr->sync)      // освобождаем текущий
       curr = next                             // переходим к следующему
       next = curr->next
   ```
   *В каждый момент времени удерживаем максимум 2 мьютекса: curr и next*

5. **Освобождение последнего узла:**
   ```
   pthread_mutex_unlock(&curr->sync)
   ```

6. **Запись результата:**
   ```
   pthread_mutex_lock(&global_mutex)
   asc_pairs = local_count
   asc_nodes_read = nodes_read
   (*counter)++
   pthread_mutex_unlock(&global_mutex)
   ```
   *global_mutex блокируется только для записи результатов*

---

## 2. Поток DESC (descending_thread) - поиск убывающих пар

**Задача:** Подсчитать пары соседних узлов, где длина строки первого узла **больше** второго.

### Этапы работы:

Алгоритм **идентичен ASC**, различие только в условии сравнения:

```c
if (strlen(curr->value) > strlen(next->value))  // вместо <
    local_count++
```

**Блокировки:**
- Head_mutex → для доступа к голове
- curr->sync и next->sync → hand-over-hand при проходе
- global_mutex → для записи результата

---

## 3. Поток EQ (equal_length_thread) - поиск пар равной длины

**Задача:** Подсчитать пары соседних узлов с **одинаковой** длиной строк.

### Этапы работы:

Алгоритм **идентичен ASC и DESC**, различие в условии:

```c
if (strlen(curr->value) == strlen(next->value))  // равенство
    local_count++
```

**Блокировки:**
- Head_mutex → для доступа к голове
- curr->sync и next->sync → hand-over-hand при проходе
- global_mutex → для записи результата

---

## 4. Потоки SWAP (swap_thread) - перестановка узлов

**Задача:** Случайным образом менять местами два соседних узла в списке.

Есть **3 независимых swap-потока**, каждый работает по одинаковому алгоритму.

### Этапы работы:

1. **Увеличение счетчика:**
   ```
   pthread_mutex_lock(&global_mutex)
   (*counter)++
   pthread_mutex_unlock(&global_mutex)
   ```

2. **Получение длины списка:**
   ```
   list_length = get_list_length(storage)  // внутри использует hand-over-hand
   if (list_length < 3)
       continue  // слишком короткий список
   ```

3. **Выбор случайной позиции:**
   ```
   pos = rand() % (list_length - 2)
   ```

4. **Доступ к голове:**
   ```
   pthread_mutex_lock(&storage->head_mutex)
   first = storage->first
   pthread_mutex_lock(&first->sync)        // блокируем первый узел
   pthread_mutex_unlock(&storage->head_mutex)
   ```

5. **Переход к позиции pos (hand-over-hand):**
   ```
   prev = NULL
   curr = first
   
   for (i = 0; i < pos; i++):
       next = curr->next
       pthread_mutex_lock(&next->sync)     // блокируем следующий
       if (prev)
           pthread_mutex_unlock(&prev->sync)  // освобождаем предыдущий
       prev = curr
       curr = next
   ```
   *В итоге: prev заблокирован (если есть), curr заблокирован*

6. **Блокировка соседних узлов для swap:**
   ```
   A = curr
   B = A->next
   pthread_mutex_lock(&B->sync)            // теперь удерживаем: prev, A, B
   ```

7. **Случайное решение о свопе:**
   ```
   should_swap = rand() % 2
   
   if (should_swap):
       if (prev == NULL):  // A был головой списка
           pthread_mutex_lock(&storage->head_mutex)
           storage->first = B
           pthread_mutex_unlock(&storage->head_mutex)
       else:
           prev->next = B
       
       A->next = B->next
       B->next = A
       
       pthread_mutex_lock(&global_mutex)
       swap_counts[thread_id]++
       pthread_mutex_unlock(&global_mutex)
   ```

8. **Освобождение всех блокировок:**
   ```
   pthread_mutex_unlock(&B->sync)
   pthread_mutex_unlock(&A->sync)
   if (prev)
       pthread_mutex_unlock(&prev->sync)
   ```

---

## Схема блокировок для каждого типа потока

### Потоки чтения (ASC, DESC, EQ):
```
[head_mutex] → [node1.sync] → [node2.sync] → ... → [global_mutex]
     ↓              ↓               ↓
  быстро       hand-over-hand   только для
 освобождается  (всегда макс.   записи
                2 узла)          результата
```

### Потоки изменения (SWAP1, SWAP2, SWAP3):
```
[global_mutex]   →   [head_mutex]   →   hand-over-hand до pos   →   
    (счетчик)         (доступ)           [prev] [A] [B]
                                              ↓    ↓   ↓
                                         swap операция
                                              ↓
                                        [head_mutex] (если надо)
                                              ↓
                                        [global_mutex]
                                         (результат)
```

---

## Ключевые моменты синхронизации

1. **head_mutex** - защищает только указатель на голову списка, быстро освобождается
2. **node.sync** - каждый узел имеет свой mutex, используется техника hand-over-hand
3. **global_mutex** - только для глобальных счетчиков и результатов, не для обхода списка
4. **Hand-over-hand блокировка** - гарантирует, что удерживаются максимум 2-3 узла одновременно
5. **Минимальное время удержания** - каждый mutex освобождается как можно быстрее
6. **Swap потоки** могут блокировать до 3 узлов (prev, A, B) + head_mutex при необходимости

Эта схема позволяет **параллельную работу** всех потоков без глобальной блокировки списка.
