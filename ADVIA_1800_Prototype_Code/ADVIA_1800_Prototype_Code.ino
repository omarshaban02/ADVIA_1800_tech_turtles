#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// Global Variables
int LDRValue = 0;               // Variable to store LDR sensor value
float reference_intensity = 0.0;  // Reference intensity for absorbance calculation

// Create objects
Servo servoMotor_up_down;            // Create a servo object for up/down movement
Servo servoMotor_right_left;         // Create a servo object for right/left movement
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize the LCD with address 0x27 and size 16x2

// Define pin connections
#define LDR_PIN 6
#define LASER_PIN 13
#define EMPTY_PUMP_RELAY 2
#define PROBE_PUMP_RELAY 3
#define SAMPLE_WATER_LEVEL A0
#define REAGENT_WATER_LEVEL A1
#define HOME_SENSOR 9

// Define constants for ON/OFF states
#define OFF LOW
#define ON HIGH
#define OFF_PROBE_PUMP HIGH
#define ON_PROBE_PUMP LOW

// Define constants for absorbance calculation
#define epsilon 85000  // Molar absorptivity (constant)
#define pathLength 13  // Path length in cm (constant)

// Define water level thresholds
#define sample_water_level_threshold 400
#define reagent_water_level_threshold 400

// Define servo angles and movement
#define up_angle 70
#define down_angle 5
#define water_cuvette 10
#define reagent_cuvette 65
#define sample_cuvette 150
#define home_position 10
#define move_angle 1  // Angle increment for servo movement

// Define delays (in milliseconds)
#define up_down_delay 30
#define right_left_delay 2000
#define probe_pump_delay 1000
#define empty_pump_delay 4000

// Function declarations
void empty(void);
void servoMotor(int cuvette);
void concentration_calculation(void);
void lcd_reset(void);
void lcd_error(String error, String error_code);
float convertToLux(int analogValue);
float calculateAbsorbance(float I0, float I);
float someFunctionOfLDRResistance(float ldrResistance);

void setup(void) {
  // Initialize servo motors
  servoMotor_up_down.attach(8);     // Attach the up/down servo to pin 8
  servoMotor_right_left.attach(5);  // Attach the right/left servo to pin 5

  // Set initial servo positions
  servoMotor_up_down.write(up_angle);
  delay(1000);
  servoMotor_right_left.write(home_position);

  // Initialize laser and sensor pins
  pinMode(LASER_PIN, OUTPUT);
  pinMode(HOME_SENSOR, INPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd_reset();

  // Initialize relays
  pinMode(EMPTY_PUMP_RELAY, OUTPUT);
  pinMode(PROBE_PUMP_RELAY, OUTPUT);
  digitalWrite(EMPTY_PUMP_RELAY, OFF);
  digitalWrite(PROBE_PUMP_RELAY, OFF_PROBE_PUMP);

  // Convert LDR value to intensity (lux)
  reference_intensity = convertToLux(analogRead(LDR_PIN));

  Serial.begin(9600);  // Start serial communication
}

void loop(void) {
  // Print sensor readings to the serial monitor
  Serial.print("SAMPLE_WATER_LEVEL = ");
  Serial.println(analogRead(SAMPLE_WATER_LEVEL));
  Serial.print("REAGENT_WATER_LEVEL = ");
  Serial.println(analogRead(REAGENT_WATER_LEVEL));
  Serial.print("home sensor = ");
  Serial.println(digitalRead(HOME_SENSOR));
  delay(1000);  // Delay for readability in serial monitor

  // Check home sensor and water levels
  if (digitalRead(HOME_SENSOR) != 0)
    lcd_error("Home Sensor", "415");

  else if (analogRead(SAMPLE_WATER_LEVEL) < sample_water_level_threshold)
  lcd_error("Sample Cuvette", "129");

  else if (analogRead(REAGENT_WATER_LEVEL) < reagent_water_level_threshold)
  lcd_error("Reagent Cuvette", "806");

  else {
    // If checks pass, proceed with operations
    servoMotor(sample_cuvette);
    
    servoMotor(reagent_cuvette);
    
    concentration_calculation();
    
    empty();
    
    servoMotor(water_cuvette);
    
    empty();
    
    servoMotor(water_cuvette);
    
    empty();
    
    lcd_reset();
  }

  servoMotor_right_left.write(home_position);

  delay(1000);
}

void concentration_calculation(void) {
  digitalWrite(LASER_PIN, ON);
  delay(1000);

  LDRValue = analogRead(LDR_PIN);
  Serial.print("LDR value :");
  Serial.println(LDRValue);
  float intensity = convertToLux(LDRValue);
// Serial.print("intensity value :");
//   Serial.println(intensity);
  // Calculate absorbance using Beer-Lambert Law
  float absorbance = calculateAbsorbance(reference_intensity, intensity);
// Serial.print("reference_intensity value :");
//   Serial.println(reference_intensity);
  // Calculate concentration
  float concentration = absorbance / (epsilon * pathLength);
  Serial.print("concentration :");
  Serial.println(concentration);
  delay(2);

  // Display concentration on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("concentration:");
  lcd.setCursor(0, 1);
  lcd.print(LDRValue);
  delay(2000);
  digitalWrite(LASER_PIN, OFF);
}

void servoMotor(int cuvette) {
  servoMotor_right_left.write(cuvette);
  Serial.print("Servo Motor right / left = ");
  Serial.println(cuvette);

  delay(right_left_delay);

  // Move the probe down
  for (int angle = up_angle; angle >= down_angle + move_angle; angle -= move_angle) {
    servoMotor_up_down.write(angle);
    delay(up_down_delay);
  }
  servoMotor_up_down.write(down_angle);
  delay(up_down_delay);
  Serial.print("Servo Motor up / down = ");
  Serial.println(down_angle);

  digitalWrite(PROBE_PUMP_RELAY, ON_PROBE_PUMP);
  Serial.println("probe pump on");
  delay(probe_pump_delay);

  // Move the probe up
  for (int angle = down_angle; angle <= up_angle - move_angle; angle += move_angle) {
    servoMotor_up_down.write(angle);
    delay(up_down_delay);
  }
  servoMotor_up_down.write(up_angle);
  delay(up_down_delay);

  digitalWrite(PROBE_PUMP_RELAY, OFF_PROBE_PUMP);
  Serial.println("probe pump off");
  delay(probe_pump_delay);

  Serial.print("Servo Motor up / down = ");
  Serial.println(up_angle);
}

void empty(void) {
  digitalWrite(EMPTY_PUMP_RELAY, ON);
  Serial.println("empty pump on");

  delay(empty_pump_delay);

  digitalWrite(EMPTY_PUMP_RELAY, OFF);
  Serial.println("empty pump off");

  delay(empty_pump_delay);
}

void lcd_reset(void) {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Welcome");
  lcd.setCursor(2, 1);
  lcd.print("Tech Turtles");
}

void lcd_error(String error, String error_code) {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Error ");
  lcd.print(error_code);
  lcd.setCursor(2, 1);
  lcd.print(error);
}

// Function to convert LDR analog value to intensity (lux)
float convertToLux(int analogValue) {
  float voltage = analogValue * (5.0 / 1023.0);
  float ldrResistance = (5.0 * ldrResistance) / (5.0 - voltage);
  float lux = someFunctionOfLDRResistance(ldrResistance);
  return lux;
}

// Function to calculate absorbance
float calculateAbsorbance(float I0, float I) {
  if (I <= 0) {
    I = 0.01;  // Avoid log(0) error
  }
  return log10(I0 / I);
}

// Function to convert LDR resistance to lux
float someFunctionOfLDRResistance(float ldrResistance) {
  // Example calibration parameters
  float m = 100.0;
  float b = 200.0;
  float lux = m * ldrResistance + b;
  return lux;
}
