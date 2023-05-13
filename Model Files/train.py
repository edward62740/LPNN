import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, InputLayer, Dropout, Conv1D, Conv2D, Flatten, Reshape, MaxPooling1D, MaxPooling2D, AveragePooling2D, BatchNormalization, TimeDistributed, Permute, ReLU, Softmax , LeakyReLU
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.callbacks import ReduceLROnPlateau, EarlyStopping
EPOCHS = 200
LEARNING_RATE = args.learning_rate or 0.0005
# this controls the batch size, or you can manipulate the tf.data.Dataset objects yourself
BATCH_SIZE = 32
train_dataset = train_dataset.batch(BATCH_SIZE, drop_remainder=False)
validation_dataset = validation_dataset.batch(BATCH_SIZE, drop_remainder=False)

# model architectureca
model = Sequential()
model.add(Dense(256, activation='relu',
    activity_regularizer=tf.keras.regularizers.l1(0.00001)))
model.add(BatchNormalization())
model.add(Dropout(0.2))
model.add(Dense(256, activation='relu',
    activity_regularizer=tf.keras.regularizers.l1(0.00001)))
model.add(BatchNormalization())
model.add(Dropout(0.2))
model.add(Dense(256, activation='relu',
    activity_regularizer=tf.keras.regularizers.l1(0.00001)))
model.add(BatchNormalization())
model.add(Dropout(0.2))
model.add(Dense(classes, name='y_pred', activation='softmax'))

# this controls the learning rate
lr_scheduler = ReduceLROnPlateau(patience=25)  # Reduce learning rate on plateau
early_stopping = EarlyStopping(patience=50)  # Early stopping if validation loss doesn't improve
opt = Adam(learning_rate=LEARNING_RATE, beta_1=0.9, beta_2=0.999)
callbacks.append(BatchLoggerCallback(BATCH_SIZE, train_sample_count, epochs=EPOCHS))
callbacks.append(lr_scheduler)
callbacks.append(early_stopping)


# train the neural network
model.compile(loss='categorical_crossentropy', optimizer=opt, metrics=['accuracy'])
model.fit(train_dataset, epochs=EPOCHS, validation_data=validation_dataset, verbose=2, callbacks=callbacks)

# Use this flag to disable per-channel quantization for a model.
# This can reduce RAM usage for convolutional models, but may have
# an impact on accuracy.
disable_per_channel_quantization = False