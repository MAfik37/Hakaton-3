// Пины светодиодной шкалы (10 штук) - порядок для визуального эффекта
const int LED_PINS[] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
const int NUM_LEDS = 10;

const int MIC_PIN = A0;
const int BTN_PIN = 12;

// Параметры (можно менять через Serial)
unsigned long measurePeriod = 10;   // Период измерения: 1-100 мс
unsigned long accumInterval = 100;  // Интервал накопления: 10-1000 мс (>= measurePeriod)

// Состояние системы
bool isRunning = false;
unsigned long lastMeasureTime = 0;
unsigned long lastAccumTime = 0;
long sensorSum = 0;
int sensorCount = 0;
int averagedAmplitude = 0;
int baseline = 512;
const int NOISE_THRESHOLD = 20;


int runnerPos = 0;
int runnerDir = 1; // 1 = вправо, -1 = влево
unsigned long lastAnimTime = 0;
const unsigned long ANIM_INTERVAL = 300; // мс между шагами анимации
int lastGraphValue = -1;

//  Переменные для обработки кнопки 
int btnState = HIGH;
int lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);
  
  // Настройка пинов светодиодов
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  
 
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  //  Автокалибровка
  Serial.println("=== Шкала уровня звука ===");
  Serial.print("Калибровка тишины... ");
  long sum = 0;
  const int samples = 150;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(MIC_PIN);
    delay(2);
  }
  baseline = sum / samples;
  Serial.println("OK (" + String(baseline) + ")");
  
  // Инициализация таймеров
  lastMeasureTime = millis();
  lastAccumTime = millis();
  lastAnimTime = millis();
  
  Serial.println("Готово. Нажмите кнопку для запуска.");
}

void loop() {
  handleSerialInput(); // 3. Обработка параметров с валидацией
  handleButton();      // 4. Ваша надёжная обработка кнопки
  
  if (isRunning) {
    unsigned long now = millis();
    
    // 1. Периодическое измерение сигнала
    if (now - lastMeasureTime >= measurePeriod) {
      int raw = analogRead(MIC_PIN);
      int amplitude = abs(raw - baseline);
      
      if (amplitude > NOISE_THRESHOLD) {
        sensorSum += amplitude;
        sensorCount++;
      }
      lastMeasureTime = now;
    }
    
    //  Усреднение и вывод
    if (now - lastAccumTime >= accumInterval) {
      averagedAmplitude = (sensorCount > 0) ? (sensorSum / sensorCount) : 0;
      sensorSum = 0;
      sensorCount = 0;
      lastAccumTime = now;
      
      updateLEDs(averagedAmplitude);
      printGraph(averagedAmplitude); // 2. Вывод графика в терминал
    }
  } else {
    // 1. Анимация "бегающего пикселя" в режиме паузы
    runPixelAnimation();
  }
}


void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;
    
  
    int spaceIdx = input.indexOf(' ');
    if (spaceIdx > 0) {
      int period = input.substring(0, spaceIdx).toInt();
      int interval = input.substring(spaceIdx + 1).toInt();
      
     
      bool periodOk = (period >= 1 && period <= 100);
      bool intervalOk = (interval >= 10 && interval <= 1000);
      bool relationOk = (interval >= period);
      
      if (periodOk && intervalOk && relationOk) {
        measurePeriod = period;
        accumInterval = interval;
        // Сброс таймеров, чтобы новые параметры применились сразу
        lastMeasureTime = millis();
        lastAccumTime = millis();
        sensorSum = 0;
        sensorCount = 0;
        
        Serial.println("✓ Параметры обновлены:");
        Serial.println("  Период: " + String(measurePeriod) + " мс");
        Serial.println("  Интервал: " + String(accumInterval) + " мс");
      } else {
        Serial.println("✖ Ошибка параметров!");
        if (!periodOk) Serial.println("  • Период должен быть 1-100 мс");
        if (!intervalOk) Serial.println("  • Интервал должен быть 10-1000 мс");
        if (!relationOk) Serial.println("  • Интервал должен быть >= периода");
      }
    } else {
      Serial.println("✖ Формат: <период> <интервал> (через пробел)");
    }
  }
}

//  Обработка кнопки 
// Цифровой фильтр: классический алгоритм подавления дребезга
void handleButton() {
  int reading = digitalRead(BTN_PIN);
  
  // Если состояние изменилось, сбрасываем таймер
  if (reading != lastBtnState) {
    lastDebounceTime = millis();
  }
  

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Если состояние действительно изменилось
    if (reading != btnState) {
      btnState = reading;
      
   
      if (btnState == LOW) {
        isRunning = !isRunning;
        Serial.println(isRunning ? "▶ Старт" : "⏹ Стоп");
        
        // При остановке сбрасываем данные
        if (!isRunning) {
          sensorSum = 0; 
          sensorCount = 0; 
          averagedAmplitude = 0;
        }
      }
    }
  }
  lastBtnState = reading;
}

//  Анимация 
void runPixelAnimation() {Ы
  unsigned long now = millis();
  
  // Обновляем позицию с заданным интервалом
  if (now - lastAnimTime >= ANIM_INTERVAL) {
    // Гасим все светодиоды
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LED_PINS[i], HIGH);
    }
    

    digitalWrite(LED_PINS[runnerPos], LOW);
    
    // Обновляем позицию и направление
    runnerPos += runnerDir;
    
    // Если дошли до края - меняем направление
    if (runnerPos >= NUM_LEDS) {
      runnerPos = NUM_LEDS - 1;
      runnerDir = -1; // назад
    } else if (runnerPos <= 0) {
      runnerPos = 0;
      runnerDir = 1; // вперед
    }
    
    lastAnimTime = now;
  }
}

void printGraph(int value) {
  // Не выводим, если значение не изменилось (чтобы не засорять порт)
  if (value == lastGraphValue) return;
  lastGraphValue = value;
  
  const int GRAPH_WIDTH = 40;
  int barLength = map(value, 0, 512, 0, GRAPH_WIDTH);
  barLength = constrain(barLength, 0, GRAPH_WIDTH);
  
  Serial.print("📊 [");
  for (int i = 0; i < GRAPH_WIDTH; i++) {
    Serial.print(i < barLength ? "█" : "░");
  }
  Serial.print("] ");
  Serial.print(value);
  Serial.print("/512 (");
  Serial.print(map(value, 0, 512, 0, 100));
  Serial.println("%)");
}

//  Вывод на светодиодную шкалу
void updateLEDs(int amplitude) {
  // 10-бит АЦП: макс. амплитуда переменного сигнала = 512
  int ledsOn = map(amplitude, 0, 512, 0, NUM_LEDS);
  ledsOn = constrain(ledsOn, 0, NUM_LEDS);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], i >= ledsOn ? HIGH : LOW);
  }
}
