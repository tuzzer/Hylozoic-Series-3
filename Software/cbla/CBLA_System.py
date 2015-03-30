import os
import threading
import glob
import pickle
import time
import shutil
import copy

from interactive_system import InteractiveCmd
from interactive_system.InteractiveCmd import command_object
from interactive_system.Messenger import Messenger
from CBLA_Engine import CBLA_Engine
from DataCollector import DataCollector

import Robot
# from SimSystem import DiagonalPlane as Robot


# ======= CBLA Engine Settings ==========
USING_SAVED_EXPERTS = False
EXPERT_FILE_NAME = None #'cbla_data_15-02-21_16-09-54.pkl'
#========================================

class CBLA_Behaviours(InteractiveCmd.InteractiveCmd):

    # ========= the Run function for the entire CBLA behaviour =====
    def run(self):

        # ====== retrieving data ======
        # change folder to "pickle_jar". This is where al the data will be stored
        curr_dir = os.getcwd()
        os.chdir(os.path.join(curr_dir, "pickle_jar"))

        # open the Data Collector
        filename = None
        data_collector = DataCollector()

        # using existing Data
        if USING_SAVED_EXPERTS:
            try:
                if EXPERT_FILE_NAME is None:
                    newest_data_file = max(glob.iglob('cbla_data*.[Pp][Kk][Ll]'), key=os.path.getctime)
                else:
                    newest_data_file = EXPERT_FILE_NAME
            except Exception:
                print("Cannot use saved data")
            else:
                try:
                    with open(newest_data_file, 'rb') as input:
                        data_import = pickle.load(input)
                except EOFError:
                    print("File Error!")
                else:
                    try:
                        data_collector = DataCollector(data_import)
                        filename = newest_data_file
                    except TypeError:
                        print("There isn't any saved data")



        # === setting up messenger ====
        messenger = Messenger(self, 0.00)

        teensy_names = self.teensy_manager.get_teensy_name_list()

        # initially update the Teensys with all the output parameters here
        self.update_output_params(teensy_names)

        # ===== setting up robots and CBLA Engines ======
        # storage for the list of CBLA Engine thread
        self.cbla_engine = dict()
        for teensy_name in teensy_names:

            # ------ set mode ------
            cmd_obj = command_object(teensy_name, 'basic')
            cmd_obj.add_param_change('operation_mode', 3)
            self.enter_command(cmd_obj)

            # ------ configuration ------
            # set the Tentacle on/off periods
            cmd_obj = command_object(teensy_name, 'tentacle_high_level')
            for j in range(3):
                device_header = 'tentacle_%d_' % j
                cmd_obj.add_param_change(device_header + 'arm_cycle_on_period', 15)
                cmd_obj.add_param_change(device_header + 'arm_cycle_off_period', 105)
            self.enter_command(cmd_obj)

            # ------ instantiate robots -------

            # ~~ Protocell (LED and ALS) ~~~~
            protocell_action = ((teensy_name, 'protocell_0_led_level', ),)
            protocell_sensor = ((teensy_name, 'protocell_0_als_state', 0, 4095),
                                (teensy_name, 'tentacle_0_ir_0_state', 0, 4095),)
            robot_led = Robot.Protocell_Node(protocell_action, protocell_sensor, messenger,
                                             sample_period=0.09, sample_interval=0.045,
                                             name=(teensy_name + '_LED'))

            # --- one tentacle arm; derived acc features ---
            robot_sma = []
            for j in range(3):
                device_header = 'tentacle_%d_' % j

                sma_action = ((teensy_name, device_header + "arm_motion_on"),)
                sma_sensor = ()
                sma_sensor_derived = ((teensy_name, device_header + 'acc_mean_x', -256, 256),
                                      (teensy_name, device_header + 'acc_mean_y', -256, 256),
                                      (teensy_name, device_header + 'acc_mean_z', -256, 256),
                                      (teensy_name, 'tentacle_0_ir_0_mean', 0, 4095), )
                sma_hidden = ((teensy_name, device_header + 'cycling'),)

                robot_sma.append(Robot.Tentacle_Arm_Node(sma_action, sma_sensor, messenger,
                                                         sample_period=0.3, sample_interval=12.0,
                                                         name=(teensy_name + '_SMA_%d' % j),
                                                         derived_vars=sma_sensor_derived, hidden_vars=sma_hidden))

            # -------- instantiate CBLA Engines ----------------
            with self.lock:

                # ~~~~ creating CBLA Engines ~~~~
                # self.cbla_engine[teensy_name + '_LED'] = CBLA_Engine(robot_led, data_collect=data_collector,
                #                                                      id=1,
                #                                                      sim_duration=float('inf'),
                #                                                      target_loop_period=0.050,
                #                                                      split_thres=100,
                #                                                      split_thres_growth_rate=1.5,
                #                                                      split_lock_count_thres=250,
                #                                                      mean_err_thres=0.0025,
                #                                                      kga_delta=10, kga_tau=30,
                #                                                      learning_rate=0.25,
                #                                                      snapshot_period=10,
                #                                                      print_to_terminal=False)
                for j in range(1):#len(robot_sma)):
                    self.cbla_engine['%s_SMA_%d' % (teensy_name, j)] = CBLA_Engine(robot_sma[j], data_collect=data_collector,
                                                                                   id=2 + j,
                                                                                   sim_duration=float('inf'),
                                                                                   target_loop_period=15,
                                                                                   split_thres=25,
                                                                                   split_thres_growth_rate=1.2,
                                                                                   split_lock_count_thres=1,
                                                                                   mean_err_thres=0.0025,
                                                                                   kga_delta=3, kga_tau=5,
                                                                                   learning_rate=0.7,
                                                                                   snapshot_period=30)

        # ~~~~ start the Messenger ~~~~
        messenger.start()

        # ~~~~ starting the CBLA engines ~~~~~
        for cbla_thread in self.cbla_engine.values():
            cbla_thread.start()


        # create new file if we didn't import data
        if filename is None:
            now = time.strftime("%y-%m-%d_%H-%M-%S", time.localtime())
            filename = 'cbla_data_%s.pkl' % now

        # input any key to key program
        kill_program = []
        threading.Thread(target=input_thread, args=(kill_program,), daemon=True).start()

        # saving the data every 2s
        end_program = False
        max_save = 200
        while not end_program:

            if kill_program:

                # killing each of the threads
                for cbla_thread in self.cbla_engine.values():
                    cbla_thread.killed = True

                # waiting for the threads to die
                for cbla_thread in self.cbla_engine.values():
                    cbla_thread.join()


                end_program = True
                #save all data
                max_save=float('inf')

            # save data
            data_collector.append(max_save=max_save)

            save_to_file(filename, data_collector.data_collection)

            #time.sleep(2)

        # remove tmp files
        temp_files = glob.glob("*.tmp")
        for temp_file in temp_files:
            os.remove(temp_file)
        print("CBLA Behaviours Finished\n\n")

        for teensy_name in tuple(teensy_names):
            self.teensy_manager.kill_teensy_thread(teensy_name)


def input_thread(L):
    input()
    L.append('Kill')

def save_to_file(filename, data):

    # create a temp file
    temp_filename = "__%s.tmp" % filename
    with open(temp_filename, 'wb') as output:
        pickle.dump(data, output, protocol=3)

        output.flush()
        os.fsync(output.fileno())

    # move original file
    shutil.copy(temp_filename, filename)
