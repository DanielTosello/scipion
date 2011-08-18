package micrographs.ctf.tasks;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author Juanjo Vega
 */
public class TasksEngine implements iTaskCompletionListener {

    private int tasks = 0;
    private iCTFGUI gui;

    public TasksEngine(iCTFGUI gui) {
        this.gui = gui;
    }

    public synchronized void add(CommandTask task) {
        gui.setRowBusy(task.row);

        tasks++;
//        System.out.println(" +++ Starting " + task.row + " / " + tasks);
        task.start();
    }

    private boolean allDone() {
        return tasks == 0;
    }

    public synchronized void done(CommandTask task) {
        if (allDone()) {    // In the end, sortPSDs is executed. After that it's done.
            gui.done();
        } else {
            tasks--;
//            System.out.println(" --- Task " + task.row + " is done! / " + tasks);
            gui.setRowIdle(task.row);

            // It's done, so sortPSDS is executed in background, and after that,
            // main GUI is updated.
            if (allDone()) {
                SortPSDSTask sortPSDS = new SortPSDSTask(gui.getFilename(), this);
                sortPSDS.start();
            }
        }
    }
}
