import tensorflow as tf
from tensorflow.keras.preprocessing import image
from tensorflow.keras import layers, models
from tensorflow.keras.preprocessing.image import ImageDataGenerator
import numpy as np
import os

# Configuración inicial
data_dir = "/path"
batch_size = 32
img_height = 224
img_width = 224

# Preprocesamiento de datos
train_datagen = ImageDataGenerator(
    rescale=1.0/255,
    validation_split=0.2,  # 80% entrenamiento, 20% validación
    rotation_range=30,
    width_shift_range=0.2,
    height_shift_range=0.2,
    shear_range=0.2,
    zoom_range=0.3,
    horizontal_flip=True,
    fill_mode="nearest"
)

train_data = train_datagen.flow_from_directory(
    data_dir,
    target_size=(img_height, img_width),
    batch_size=batch_size,
    class_mode="binary",  # Dos categorías
    subset="training"
)

val_data = train_datagen.flow_from_directory(
    data_dir,
    target_size=(img_height, img_width),
    batch_size=batch_size,
    class_mode="binary",
    subset="validation"
)

# Usar MobileNetV2 como base
base_model = tf.keras.applications.MobileNetV2(input_shape=(img_height, img_width, 3), include_top=False, weights='imagenet')
base_model.trainable = True

# Congelar las primeras 100 capas
for layer in base_model.layers[:100]:
    layer.trainable = False

model = tf.keras.Sequential([
    base_model,
    tf.keras.layers.GlobalAveragePooling2D(),
    tf.keras.layers.Dense(1, activation='sigmoid')  # Para clasificación binaria
])

model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=1e-4), loss='binary_crossentropy', metrics=['accuracy'])
model.fit(train_data, validation_data=val_data, epochs=20)  # Aumentar las épocas

model.save("mascotas_modelo.keras")

loss, accuracy = model.evaluate(val_data)
print(f"Precisión en validación: {accuracy*100:.2f}%")


model = tf.keras.models.load_model('mascotas_modelo.keras')

# Tamaño de la imagen
img_height = 224
img_width = 224

test_images_dir = '/path'

# Obtener la lista de imágenes en la carpeta
image_files = [f for f in os.listdir(test_images_dir) if f.endswith(('.jpg', '.jpeg', '.png', '.jfif'))]

# Definir las clases
class_names = {0: 'Perro', 1: 'Oso'}

# Umbral para decidir si una imagen no es reconocida
confidence_threshold = 0.90

# Evaluar cada imagen
for image_file in image_files:
    # Ruta completa de la imagen
    img_path = os.path.join(test_images_dir, image_file)

    # Cargar y preprocesar la imagen
    img = image.load_img(img_path, target_size=(img_height, img_width))
    img_array = image.img_to_array(img)
    img_array = np.expand_dims(img_array, axis=0)  # Añadir una dimensión extra para el batch
    img_array = img_array / 255.0  # Normalizar la imagen

    # Hacer una predicción
    prediction = model.predict(img_array)[0][0]  # Predicción única (valor entre 0 y 1)

    # Determinar la clase y la confianza
    predicted_class = 1 if prediction >= 0.5 else 0
    confidence = prediction if predicted_class == 1 else 1 - prediction

    # Mostrar resultados
    if confidence >= confidence_threshold:
        print(f'Imagen: {image_file}')
        print(f'Predicción: {class_names[predicted_class]} (Confianza: {confidence:.2f})')
    else:
        print(f'Imagen: {image_file}')
        print(f'Predicción: No reconocido (Confianza: {confidence:.2f})')




from google.colab import drive
drive.mount('/path')
import time
from shutil import move
from PIL import Image

input_folder_izq = '/path'
processed_folder_izq = '/path'

input_folder_der = '/path'
processed_folder_der = '/path'


import simpleaudio as sa
import gspread
from oauth2client.service_account import ServiceAccountCredentials
import time

sono_audio_izq = False;

sono_audio_der = False;

# Cargar los archivos de audio
audio_izq = "/path" 
audio_der = "/path"  


# Función para reproducir audio
def play_audio(audio_file):
    wave_obj = sa.WaveObject.from_wave_file(audio_file)
    play_obj = wave_obj.play()
    play_obj.wait_done()  # Espera a que termine la reproducción


from concurrent.futures import ThreadPoolExecutor
from shutil import move
import matplotlib.pyplot as plt
import gspread
from google.colab import auth
from google.auth.transport.requests import Request
from google.oauth2.service_account import Credentials
from tensorflow.keras.applications.mobilenet import preprocess_input, decode_predictions

auth.authenticate_user()

from google.auth import default
creds, _ = default()

client = gspread.authorize(creds)
spreadsheet_url = ''
sheet = client.open_by_url(spreadsheet_url).sheet1


confidence_threshold = 0.98
processed_images = set()


print(f"Valor en (12, 2): {sheet.cell(12, 2).value}, Tipo: {type(sheet.cell(12, 2).value)}")
print(f"Valor en (9, 2): {sheet.cell(9, 2).value}, Tipo: {type(sheet.cell(9, 2).value)}")


def process_image(image_name, input_folder, processed_folder, target_cell):
    global processed_images

    # Ruta de la imagen
    image_path = os.path.join(input_folder, image_name)

    # Cargar y preprocesar la imagen
    img = image.load_img(image_path, target_size=(240, 240))
    img_array = image.img_to_array(img)
    img_array = np.expand_dims(img_array, axis=0)
    img_array = preprocess_input(img_array)

    # Hacer una predicción
    predictions = model.predict(img_array)
    predicted_class = np.argmax(predictions, axis=1)
    max_confidence = np.max(predictions)

    # Actualizar la hoja de cálculo según la predicción
    if max_confidence > confidence_threshold:
        print(f'Imagen: {image_name}')
        print(f'Predicción: {class_names[predicted_class[0]]} (Confianza: {max_confidence:.2f})')
        if predicted_class[0] == 0 and input_folder == input_folder_izq:
            sheet.update(target_cell, [["1"]])
        elif predicted_class[0] == 1 and input_folder == input_folder_der:
            sheet.update(target_cell, [["1"]])
        else:
            sheet.update(target_cell, [["0"]])

    else:
        print(f'Imagen: {image_name}')
        print(f'Predicción: No reconocido (Confianza: {max_confidence:.2f})')
        sheet.update(target_cell, [["0"]])

    # Mover la imagen procesada a la carpeta correspondiente
    processed_path = os.path.join(processed_folder, image_name)
    move(image_path, processed_path)
    processed_images.add(image_name)
    print(f"{image_name} procesada y movida a {processed_folder}.")

# Configurar el procesamiento en paralelo para que tarde menos
with ThreadPoolExecutor(max_workers=4) as executor:
    while True:
        # Obtener imágenes nuevas de ambas carpetas
        all_images_izq = [f for f in os.listdir(input_folder_izq) if f.endswith(('.jpg', '.png'))]
        new_images_izq = [img for img in all_images_izq if img not in processed_images]

        #all_images_der = [f for f in os.listdir(input_folder_der) if f.endswith(('.jpg', '.png'))]
        #new_images_der = [img for img in all_images_der if img not in processed_images]

        # Procesar las imágenes nuevas en paralelo
        for img in new_images_izq:
            executor.submit(process_image, img, input_folder_izq, processed_folder_izq, 'E2')

        #for img in new_images_der:
        #   executor.submit(process_image, img, input_folder_der, processed_folder_der, 'E3')
